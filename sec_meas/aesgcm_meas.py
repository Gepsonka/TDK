import time
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

key = open('aes-gcm-key.bin', 'rb').read()  # Replace with your secret key
iv = open('aes-gcm-iv.bin', 'rb').read()  # Replace with your initialization vector


plaintext = open('random_file', 'rb').read()  # Replace with your plaintext

cipher = Cipher(
    algorithms.AES(key),
    modes.GCM(iv),
)
encryptor = cipher.encryptor()

st = time.process_time()
ciphertext = encryptor.update(plaintext) + encryptor.finalize()
et = time.process_time()

print(f"Time taken to encrypt: {et-st} seconds")

# The tag is needed for decryption and authentication
tag = encryptor.tag

cipher = Cipher(
    algorithms.AES(key),
    modes.GCM(iv, tag),
)

decryptor = cipher.decryptor()
st = time.process_time()
decrypted_text = decryptor.update(ciphertext) + decryptor.finalize()
et = time.process_time()

print(f"Time taken to decrypt: {et-st} seconds")

try:
    decryptor.authenticate_additional_data(tag)
    print("Authentication successful")
except:
    print("Authentication failed - the data may have been tampered with!")

