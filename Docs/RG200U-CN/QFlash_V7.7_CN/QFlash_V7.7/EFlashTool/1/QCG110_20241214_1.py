import os
import argparse
import serial
import time
  
MAX_RETRY = 5
RETRY_DELAY = 2
FW_FILE_OFFSET = 512

#ACK response codes
ACK_TRUE           = True
ACK_FALSE          = False
    
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
            #custom_string = f'AT+QLOCTEST = "locWriteDataToQCX217NVM dataSize {chunk_size} data {data_chunk}"\r\n'
            custom_string = f'AT$QCLOCGNSSPUSHFWBIN ={chunk_size}, {data_chunk}\r\n'
            ser.write(custom_string.encode())
            ser.flush()
            ser.close()
            print(f"Block {block_number} send success")
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
                print(f"[RX]: {rec_data}")
                if "$QCLOCGNSSPUSHFWBIN: SUCCESS" in rec_data:
                    ser.close()
                    return ACK_TRUE                   
                elif "$QCLOCGNSSTRIGGERFWUP: FWUP SUCCESS" in rec_data:
                    ser.close()
                    return ACK_TRUE
                elif "$QCLOCGNSSGETFWVERSION: SW version" in rec_data:
                    ser.close()
                    return ACK_TRUE        
                elif "$QCLOCGNSSTRIGGERFWUP: FWUP FAIL" in rec_data:
                    ser.close()
                    return ACK_FALSE                 
                elif "+CME ERROR" in rec_data:
                    ser.close()
                    return ACK_FALSE
                                   
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
            print(f"[TX]: {at_command} ")
            return
        except serial.SerialException as e:
            num_retries += 1
            print(f"sendAT_command serial port error : {e}")
            print(f"sendAT_command Retry {num_retries}/{MAX_RETRY}")
            time.sleep(RETRY_DELAY)
    
    
def main():

    chunk_size = 1024           #for 1KB data, this script will send almost 5226 Bytes, as each character is transmitted as a byte

    parser = argparse.ArgumentParser(description = 'NULL')
    parser.add_argument("--serialPort", type = str, help = 'Enter the serial port.')
    parser.add_argument('--filePath', type = str, help = 'Enter the file path to check.')
    parser.add_argument('--triggerFWUP', type = str, help = 'Set this to True for FWUP, False otherwise and in default case.')
    
    args = parser.parse_args()
    COM = args.serialPort
    filePath = args.filePath
    fileSize = 0
    isTriggerFWUP = False
        
    if args.triggerFWUP:
        if args.triggerFWUP == 'True':
            isTriggerFWUP = True
    
    if os.path.exists(filePath):
        fileName = os.path.basename(filePath)
        if fileName.lower().endswith('.bin'):
            print(f'Valid {fileName} file')
            fileSize = os.path.getsize(filePath)         
        else:
            print(f'{fileName} is not a .bin file')
            sys.exit()
    else:
        print(f'The file path {filePath} is invalid.')
        sys.exit()
                  
    #reset QCX217
    sendAT_command(f'AT$QCRST', COM)
    time.sleep(5)        
    
    #clear FOTA region
    sendAT_command(f'AT$QCLOCGNSSFWUPOPTIONS=2', COM)
    time.sleep(2)
    #write FOTA metadata
    sendAT_command(f'AT$QCLOCGNSSFWUPOPTIONS = {1},{FW_FILE_OFFSET},{fileSize}', COM)
    time.sleep(2)
    #Load FW binary to QCX217 NVM
    result = read_and_push_data(filePath, COM, chunk_size)

    if result:
        print('File transfer success!')
        if isTriggerFWUP == True:
            print(f"############################################################")
            print(f"Starting FWUP for QCG110")
            sendAT_command(f'AT$QCLOCGNSSGETFWVERSION', COM)
            get_ack_from_serial(COM, 15)
            sendAT_command(f'AT$QCLOCGNSSTRIGGERFWUP', COM)
            get_ack_from_serial(COM, 50)
            sendAT_command(f'AT$QCLOCGNSSGETFWVERSION', COM)
            get_ack_from_serial(COM, 15)
            print(f"############################################################")
    else:
        print('File transfer fail!')
        
if __name__ == "__main__":
    main()