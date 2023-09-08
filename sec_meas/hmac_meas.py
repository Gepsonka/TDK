import hmac
import hashlib
import time

# Define your secret key and the file path
secret_key = b'd8b66c54b3f5cd95115734d90b7c512504d71ce66711ea4ced2f5b41021a4313'  # Replace with your secret key
file_path = 'random_file'  # Replace with the path to your 2KB file

# Function to calculate the HMAC of a file
def calculate_file_hmac(file_path, secret_key):
    try:
        # Create an HMAC object with the SHA-256 hash function and the secret key
        h = hmac.new(secret_key, digestmod=hashlib.sha256)
        f = open(file_path, 'rb')
        # Open the file in binary mode and read it in chunks
        
        while True:
            chunk = f.read(1024)  # Read 1KB at a time
            if not chunk:
                break
            h.update(chunk)

        # Calculate the HMAC and return it as a hexadecimal string
        st = time.process_time()
        hmac_val = h.hexdigest()
        et = time.process_time()

        print(f"Time taken to calculate HMAC: {et-st} seconds")
        return hmac_val

    except Exception as e:
        return str(e)

# Calculate and print the HMAC
file_hmac = calculate_file_hmac(file_path, secret_key)
print("HMAC:", file_hmac)