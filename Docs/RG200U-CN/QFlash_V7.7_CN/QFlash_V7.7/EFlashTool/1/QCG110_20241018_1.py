import os
import argparse
import serial
import time

#sample command to trigger this python script
#python QCG110_FWUP_via_qloctest.py C:\Scripts\E2E_FWUP\builder-tracker-qti-7e36de38-gnrel8140ldo_d1.bin COM6
    
MAX_RETRY = 5
RETRY_DELAY = 2

#ACK response codes
ACK_TRUE           = True
ACK_FALSE          = False
ACK_FWUP_SUCCESS   = 2
ACK_FWUP_FAIL      = 3
    
def read_data_chunk(file_path, chunk_size):
    try:
        with open(file_path, 'rb') as file:
            while True:
                data_chunk = file.read(chunk_size)
                if not data_chunk:
                    break
                    
                yield data_chunk
    except FileNotFoundError:
        print("file not found")

def format_bytes_as_hex(data):
    return "_".join(f"0x{byte:02X}" for byte in data)

def push_data_serial(data_chunk, chunk_size, block_number, serial_port):
    num_retries = 0
    while num_retries < MAX_RETRY:
        try:
            ser = serial.Serial(serial_port, baudrate = 115200, timeout=1) 
            custom_string = f'AT+QLOCTEST = "locWriteDataToQCX217NVM dataSize {chunk_size} data {data_chunk}"\r\n'
            ser.write(custom_string.encode())
            ser.flush()
            ser.close()
            print(f"Block {block_number} custom_string {custom_string} send success")
            return
        except serial.SerialException as e:
            num_retries += 1
            print(f"push_data serial port error : {e}")
            print(f"push_data Retry {num_retries}/{MAX_RETRY}")
            time.sleep(RETRY_DELAY)
              
def get_ack_from_serial(serial_port, timeout):
    num_retries = 0
    while num_retries < MAX_RETRY:
        try:
            ser = serial.Serial(serial_port, baudrate = 115200, timeout=5) 
            start_time = time.time()
            
            while ((time.time() - start_time) < timeout):
                rec_data = ser.readline().decode('utf-8').strip()
                print(f"Received data from qloctest: {rec_data}")
                if "LOC_AT: OK" in rec_data:
                    ser.close()
                    return ACK_TRUE
                elif "LOC_AT: NOK" in rec_data:
                    ser.close()
                    return ACK_FALSE
                elif "LOC_AT: QCG110 firmware upgrade success!!" in rec_data:
                    ser.close()
                    return ACK_FWUP_SUCCESS
                elif "LOC_AT: QCG110 firmware upgrade fail!!" in rec_data:
                    ser.close()
                    return ACK_FWUP_FAIL
                                   
            ser.close()
            return ACK_FALSE
        except serial.SerialException as e:
            num_retries += 1
            print(f"get_ack serial port error : {e}")
            print(f"get_ack Retry {num_retries}/{MAX_RETRY}")
            time.sleep(RETRY_DELAY)

def read_and_push_data(file_path, serial_port, chunk_size):
    block_number = 0
    for data_chunk in read_data_chunk(file_path, chunk_size):
        block_number += 1
        size = len(data_chunk) # this will get exact number of bytes read from file
        fmt_data = format_bytes_as_hex(data_chunk)
        push_data_serial(fmt_data, size, block_number, serial_port)
            
        #wait for ack
        ack_rec = get_ack_from_serial(serial_port, 20)
        
        if ack_rec == ACK_TRUE:
            print("Data Chunk process success, ACK received")
        else:
            print("ACK not received, terminating script!!")
            return False;
            
    print("Data transfer complete")
    return True
    
def sendAT_command(at_command, serial_port):
    num_retries = 0
    while num_retries < MAX_RETRY:
        try:
            cmd = at_command + '\r\n'
            ser = serial.Serial(serial_port, baudrate = 115200, timeout=1) 
            ser.write(cmd.encode())
            ser.flush()
            ser.close()
            print(f"AT command: {at_command} send success")
            return
        except serial.SerialException as e:
            num_retries += 1
            print(f"sendAT_command serial port error : {e}")
            print(f"sendAT_command Retry {num_retries}/{MAX_RETRY}")
            time.sleep(RETRY_DELAY)

def  get_user_details():
    parser = argparse.ArgumentParser(description = 'Check if file path is valid.')
    parser.add_argument('file_path', type = str, help = 'Enter the file path to check.')
    parser.add_argument('serial_port', type = str, help = 'Enter the serial port.')
    
    args = parser.parse_args()
    file_path = args.file_path
    serial_port = args.serial_port
    
    file_size = os.path.getsize(file_path)   
    return file_path,file_size,serial_port
    
    
def main():

    chunk_size = 1024           #for 1KB data, this script will send almost 5226 Bytes, as each character is transmitted as a byte

    #get FWUP related details 
    locFWbinFile, locFWbinSize, locSerialPort = get_user_details()
    print(f'FWUP details: BinFile: {locFWbinFile} BinFileSize: {locFWbinSize} COM port: {locSerialPort}')

    #reset QCX217
    #sendAT_command(f'AT$QCRST', locSerialPort)
    #time.sleep(25)
    
    #Load FW binary to QCX217 NVM
    result = read_and_push_data(locFWbinFile, locSerialPort, chunk_size)

    #Trigger FWUP
    if result == False:
        #exit here
        quit()
    elif result == True:
        print(f"############################################################")
        print(f"Starting FWUP for QCG110")
        sendAT_command(f'AT+QLOCTEST="locInit"', locSerialPort)
        time.sleep(5)
        sendAT_command(f'AT+QLOCTEST="locGetFWVersion"', locSerialPort)
        get_ack_from_serial(locSerialPort, 10)
        sendAT_command(f'AT+QLOCTEST="locTriggerFWUP fwBinarySize {locFWbinSize}"', locSerialPort)
        print(f"Trigger FWUP success!")
            
    print(f"Started FWUP for QCG110, please wait...")
    ack_rec2 = get_ack_from_serial(locSerialPort, 10000)
    if ack_rec2 == ACK_FWUP_SUCCESS:
        print(f"FWUP success for QCG110!!")
        sendAT_command(f'AT+QLOCTEST="locGetFWVersion"', locSerialPort)
        get_ack_from_serial(locSerialPort, 10)
    elif ack_rec2 == ACK_FWUP_FAIL:
        print(f"FWUP fail for QCG110!!")    
            
    print(f"############################################################")
    
if __name__ == "__main__":
    main()