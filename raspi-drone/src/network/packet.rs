use std::error::Error;
use std::fmt::{Display, Formatter};
use aes_gcm::aead::consts::U12;
use aes_gcm::aead::{AeadMut, Aead};
use aes_gcm::{Key, KeyInit, Nonce, Aes128Gcm};


/// max num of bytes in a LoRa packet
pub const MAX_PACKET_SIZE: usize = 255;
pub const HEADER_SIZE: usize = 7;
/// MAX_PACKET_SIZE - header - payload crc size
pub const MAX_PAYLOAD_SIZE: usize = MAX_PACKET_SIZE - HEADER_SIZE - 2;

/// = packet max size (MAX_PAYLOAD_SIZE) - nonce size (12)
pub const MAX_MESSAGE_SLICE_SIZE: usize = MAX_PAYLOAD_SIZE - 12;



#[derive(Debug)]
pub struct PacketPayloadSizeError {
    payload_size: u8,
}

impl PacketPayloadSizeError {
    fn new(payload_size: u8) -> Self {
        PacketPayloadSizeError{
            payload_size
        }
    }
}


impl Display for PacketPayloadSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Payload size must me smaller than {}! ({})", MAX_PAYLOAD_SIZE, self.payload_size)
    }
}

impl Error for PacketPayloadSizeError {}


#[derive(Debug)]
pub struct PacketSizeError {
    packet_size: usize
}


impl PacketSizeError {
    pub fn new(packet_size: usize) -> Self {
        PacketSizeError{
            packet_size
        }
    }
}

impl Display for PacketSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Packet size must me smaller than {}! ({})", MAX_PACKET_SIZE, self.packet_size)
    }
}

impl Error for PacketSizeError {}


#[derive(Debug)]
pub struct PacketEncryptionError {}

impl Display for PacketEncryptionError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Packet encryption error")
    }
}

impl Error for PacketEncryptionError {}


#[derive(Debug)]
pub struct PacketDecryptionError {}

impl Display for PacketDecryptionError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Packet decryption error")
    }
}

impl Error for PacketDecryptionError {}



#[derive(Debug)]
pub struct CRCError {}

impl Display for CRCError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "CRC mismatch")
    }
}

impl Error for CRCError {}


pub trait PacketHeaderInit {
    fn new_from_slice(slice: [u8; HEADER_SIZE]) -> Result<Self, Box<dyn std::error::Error>> where Self: Sized;
}

pub trait PacketPayloadInit {
    fn new(payload: Vec<u8>) -> Result<Self, PacketPayloadSizeError> where Self: Sized;
    fn new_from_slice(slice: &[u8]) -> Result<Self, Box<dyn std::error::Error>> where Self: Sized;
}

/// Every network communication is done via packets.
/// This trait defines the basic functionality of a packet header
/// required for the network communication of drones.
/// Each packet must have a header and payload CRC to ensure data integrity,
/// since every packet is sent through a wireless medium.
pub trait PacketHeaderCRC {
    fn calculate_header_crc(&mut self);
    fn check_header_crc(&self) -> bool;
}


pub trait PacketPayloadCRC {
    fn calculate_payload_crc(&mut self) -> Result<(), Box<dyn std::error::Error>>;
    fn check_payload_crc(&self) -> Result<bool, Box<dyn std::error::Error>>;
}


pub trait PacketEncrypt<AesGcm>
where AesGcm: AeadMut + KeyInit
{
    fn encrypt(&mut self, key: &Key<AesGcm>, nonce: &Nonce<AesGcm::NonceSize>) -> Result<(), Box<dyn Error>>;  
}

pub trait PacketDecrypt<AesGcm>
where AesGcm: AeadMut + KeyInit
{
    fn decrypt(&mut self, key: &Key<AesGcm>) -> Result<(), Box<dyn std::error::Error>>;
}

pub trait PacketField {

    type AddressSize;
    /// For specifying the max paylaod count size in a message
    type PayloadCount;
    type PayloadSize;
    type CrcSize;

    type Payload;

    fn get_source_address(&self) -> Self::AddressSize;
    
    fn get_destination_address(&self) -> Self::AddressSize;

    fn get_message_packet_num(&self) -> Self::PayloadCount;

    fn get_total_number_of_packets(&self) -> Self::PayloadCount;

    fn get_payload_size(&self) -> Self::PayloadSize;

    fn get_header_crc(&self) -> Self::CrcSize;

    fn get_payload_crc(&self) -> Self::CrcSize;

    fn get_payload(&self) -> Self::Payload;

    fn get_payload_ref(&self) -> &Self::Payload;

    fn get_payload_mut(&mut self) -> &mut Self::Payload;
}



#[derive(Debug, Default, Copy, Clone, PartialEq, Eq)]
pub struct LoRaPacketHeader {
    pub source_addr: u8,
    pub destination_addr: u8,
    pub message_packet_num: u8,
    pub total_number_of_packets: u8,
    pub payload_size: u8,
    pub header_crc: u16,
}

impl PacketHeaderInit for LoRaPacketHeader {
    fn new_from_slice(slice: [u8; HEADER_SIZE]) -> Result<LoRaPacketHeader, Box<dyn Error>> {
        if slice.len() != HEADER_SIZE {
            return Err(Box::new(PacketSizeError::new(slice.len())));
        }

        let mut header = LoRaPacketHeader::default();
        header.source_addr = slice[0];
        header.destination_addr = slice[1];
        header.message_packet_num = slice[2];
        header.total_number_of_packets = slice[3];
        header.payload_size = slice[4];
        header.header_crc = u16::from_be_bytes([slice[5], slice[6]]);


        Ok(header)
    }
}

impl PacketHeaderCRC for LoRaPacketHeader {
    fn calculate_header_crc(&mut self) {
        let header: Vec<u8> = self.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        self.header_crc = X25.checksum(&header.as_slice()[0..=4]);
    }

    fn check_header_crc(&self) -> bool {
        let header: Vec<u8> = self.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        X25.checksum( &header.as_slice()[0..=4]) == self.header_crc
    }
}


impl TryFrom<Vec<u8>> for LoRaPacketHeader {
    type Error = PacketSizeError;

    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        if value.len() != HEADER_SIZE {
            Err(Self::Error::new(value.len()))
        } else {
            Ok(LoRaPacketHeader {
                source_addr: value[0],
                destination_addr: value[1],
                message_packet_num: value[2],
                total_number_of_packets: value[3],
                payload_size: value[4],
                header_crc: ((value[5] as u16) << 8) | value[6] as u16
            })
        }
    }
}




impl Into<[u8; 7]> for LoRaPacketHeader {
    fn into(self) -> [u8; 7] {
        [
            self.source_addr,
            self.destination_addr,
            self.message_packet_num,
            self.total_number_of_packets,
            self.payload_size,
            ((self.header_crc >> 8) & 0xFF) as u8,
            (self.header_crc & 0xFF) as u8
        ]
    }
}

impl Into<Vec<u8>> for LoRaPacketHeader {
    fn into(self) -> Vec<u8> {
        vec![
            self.source_addr,
            self.destination_addr,
            self.message_packet_num,
            self.total_number_of_packets,
            self.payload_size,
            ((self.header_crc >> 8) & 0xFF) as u8,
            (self.header_crc & 0xFF) as u8
        ]
    }
}

impl Into<Vec<u8>> for &LoRaPacketHeader {
    fn into(self) -> Vec<u8> {
        vec![
            self.source_addr,
            self.destination_addr,
            self.message_packet_num,
            self.total_number_of_packets,
            self.payload_size,
            ((self.header_crc >> 8) & 0xFF) as u8,
            (self.header_crc & 0xFF) as u8
        ]
    }
}

impl Into<Vec<u8>> for &mut LoRaPacketHeader {
    fn into(self) -> Vec<u8> {
        vec![
            self.source_addr,
            self.destination_addr,
            self.message_packet_num,
            self.total_number_of_packets,
            self.payload_size,
            ((self.header_crc >> 8) & 0xFF) as u8,
            (self.header_crc & 0xFF) as u8
        ]
    }
}

#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct LoRaPacketPayload {
    pub payload_crc: u16,
    pub payload: Vec<u8>,
}

impl PacketPayloadInit for LoRaPacketPayload {
    /// Does not calculates the CRC, it initiates it to 0
    fn new(payload: Vec<u8>) -> Result<LoRaPacketPayload, PacketPayloadSizeError> {
        if payload.len() > MAX_PAYLOAD_SIZE {
            return Err(PacketPayloadSizeError::new(payload.len() as u8));
        }

        Ok(LoRaPacketPayload {
            payload_crc: 0,
            payload,
        })
    }

    fn new_from_slice(slice: &[u8]) -> Result<LoRaPacketPayload, Box<dyn Error>> {
        if slice.len() > MAX_PAYLOAD_SIZE {
            return Err(Box::new(PacketSizeError::new(slice.len())));
        }

        let mut payload = LoRaPacketPayload::default();
        payload.payload_crc = 0;
        payload.payload = slice.to_vec();

        Ok(payload)
    }
}

impl TryFrom<Vec<u8>> for LoRaPacketPayload {
    type Error = PacketSizeError;
    /// First two bytes of @value must be the crc then comes the payload
    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        if value.len() > MAX_PAYLOAD_SIZE {
            Err(PacketSizeError::new(value.len()))
        } else {
            let payload_crc = ((value[0] as u16) << 8) | value[1] as u16;
            let payload = value[2..value.len()].to_vec();

            Ok(LoRaPacketPayload {
                payload_crc,
                payload,
            })
        }

    }
}



impl Into<Vec<u8>> for LoRaPacketPayload {
    fn into(mut self) -> Vec<u8> {
        let mut payload_vec: Vec<u8> = vec![((self.payload_crc >> 8) & 0xFF) as u8,
                                            (self.payload_crc & 0xFF) as u8];
        payload_vec.append(&mut self.payload);

        payload_vec

    }
}

#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct LoRaPacket {
    pub header: LoRaPacketHeader,
    pub payload: LoRaPacketPayload
}

impl LoRaPacket {
    pub fn new(header: LoRaPacketHeader, payload: LoRaPacketPayload) -> Self {
        LoRaPacket {
            header,
            payload,
        }
    }
}

impl PacketField for LoRaPacket {
    type AddressSize = u8;
    type PayloadCount = u8;
    type PayloadSize = u8;
    type CrcSize = u16;
    type Payload = Vec<u8>;

    fn get_source_address(&self) -> Self::AddressSize {
        self.header.source_addr
    }

    fn get_destination_address(&self) -> Self::AddressSize {
        self.header.destination_addr
    }

    fn get_message_packet_num(&self) -> Self::PayloadCount {
        self.header.message_packet_num
    }

    fn get_total_number_of_packets(&self) -> Self::PayloadCount {
        self.header.total_number_of_packets
    }

    fn get_payload_size(&self) -> Self::PayloadSize {
        self.header.payload_size
    }

    fn get_header_crc(&self) -> Self::CrcSize {
        self.header.header_crc
    }

    fn get_payload_crc(&self) -> Self::CrcSize {
        self.payload.payload_crc
    }

    fn get_payload(&self) -> Self::Payload {
        self.payload.payload.clone()
    }

    fn get_payload_ref(&self) -> &Self::Payload {
        &self.payload.payload
    }

    fn get_payload_mut(&mut self) -> &mut Self::Payload {
        &mut self.payload.payload
    }

   
}

impl PacketEncrypt<Aes128Gcm> for LoRaPacket
{
    fn encrypt(&mut self, key: &Key<Aes128Gcm>, nonce: &Nonce<U12>) -> Result<(), Box<dyn Error>>
    {
        let packet_content = self.payload.payload.clone();
        let cipher = Aes128Gcm::new(key);
        let ciphertext = cipher.encrypt(nonce, self.payload.payload.as_slice()).map_err(|_e| PacketEncryptionError {})?;

        self.payload.payload = ciphertext;
        self.payload.payload.append(nonce.as_slice().to_vec().as_mut());

        if self.payload.payload.len() > MAX_PAYLOAD_SIZE {
            self.payload.payload = packet_content;
            return Err(Box::new(PacketSizeError::new(self.payload.payload.len())));
        }

        Ok(())
    }
}

impl PacketDecrypt<Aes128Gcm> for LoRaPacket
{
    fn decrypt(&mut self, key: &Key<Aes128Gcm>) -> Result<(), Box<dyn Error>>{
        let cipher = Aes128Gcm::new(key);
        let packet_content = self.payload.payload.clone();
        let mut ciphertext = self.payload.payload.clone();

        let mut nonce_slice = [0u8; 12];
        nonce_slice.copy_from_slice(&packet_content[packet_content.len() - 12..packet_content.len()]);
        let nonce = Nonce::from_slice(&nonce_slice);

        ciphertext.truncate(ciphertext.len() - 12);

        self.payload.payload = cipher.decrypt(&nonce, ciphertext.as_slice()).map_err(|_e| PacketDecryptionError {})?;


        Ok(())
    }

}

impl Into<Vec<u8>> for LoRaPacket {
    fn into(self) -> Vec<u8> {
        let mut raw_packet: Vec<u8> = self.header.into();
        raw_packet.append(&mut self.payload.into());

        raw_packet
    }
}

impl TryFrom<Vec<u8>> for LoRaPacket {
    type Error = PacketSizeError;

    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        let header = LoRaPacketHeader::try_from(value[0..=6].to_vec())?;
        let payload = LoRaPacketPayload::try_from(value[7..value.len()].to_vec())?;

        Ok(LoRaPacket {
            header,
            payload,
        })
    }
}


