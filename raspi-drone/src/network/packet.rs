use std::error::Error;
use std::fmt::{Display, Formatter};
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::{Key, KeyInit, KeySizeUser, Nonce};
use aes_gcm::aead::AeadMut;
use log::debug;

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
pub struct CRCError {}

impl Display for CRCError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "CRC mismatch")
    }
}

impl Error for CRCError {}


pub trait PacketHeaderInit {
    fn new_from_slice(slice: &[u8]) -> Result<Self, Box<dyn std::error::Error>> where Self: Sized;
}

pub trait PacketPayloadInit {
    fn new(payload: Vec<u8>) -> Self where Self: Sized;
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


pub trait PacketEncrypt<AesGcm, KeySize, NonceSize>
where NonceSize: ArrayLength<u8>,
      KeySize: KeySizeUser,
      AesGcm: AeadMut + KeyInit
{
    fn encrypt(&mut self, key: &Key<KeySize>, nonce: &Nonce<NonceSize>) -> Result<(), Box<dyn std::error::Error>>;
}

pub trait PacketDecrypt<AesGcm, KeySize, NonceSize>
where KeySize: KeySizeUser,
      NonceSize: ArrayLength<u8>,
        AesGcm: AeadMut + KeyInit
{
    fn decrypt(&mut self, key: &Key<KeySize>, nonce: &Nonce<NonceSize>) -> Result<(), Box<dyn std::error::Error>>;
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
    fn new_from_slice(slice: &[u8]) -> Result<LoRaPacketHeader, Box<dyn Error>> {
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
    fn new(payload: Vec<u8>) -> LoRaPacketPayload {
        LoRaPacketPayload {
            payload_crc: 0,
            payload,
        }
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
    pub(crate) header: LoRaPacketHeader,
    payload: LoRaPacketPayload
}

impl LoRaPacket {
    pub fn new(header: LoRaPacketHeader, payload: LoRaPacketPayload) -> Self {
        LoRaPacket {
            header,
            payload,
        }
    }
}

impl <AesGcm, KeySize, NonceSize> PacketEncrypt<AesGcm, KeySize, NonceSize> for LoRaPacket
where NonceSize: ArrayLength<u8>,
      KeySize: KeySizeUser,
      AesGcm: AeadMut + KeyInit,
{
    fn encrypt(&mut self, key: &Key<KeySize> , nonce: &Nonce<NonceSize>) -> Result<(), Box<dyn Error>> {
        let mut cipher = AesGcm::new(key);
        let mut ciphertext = cipher.encrypt(nonce, &self.payload.payload.as_slice())?;
        if ciphertext.len() > MAX_MESSAGE_SLICE_SIZE {
            debug!("Ciphertext length is too long: {}", ciphertext.len());
            return Err(Box::new(PacketSizeError::new(ciphertext.len())));
        }
        self.payload.payload = nonce.to_vec();
        self.payload.payload.append(&mut ciphertext);
        Ok(())
    }
}

impl <AesGcm, KeySize, NonceSize> PacketDecrypt<AesGcm, KeySize, NonceSize> for LoRaPacket
    where NonceSize: ArrayLength<u8>,
          KeySize: KeySizeUser,
          AesGcm: AeadMut + KeyInit,
{
    fn decrypt(&mut self, key: &Key<KeySize> , nonce: &Nonce<NonceSize>) -> Result<(), Box<dyn Error>> {
        let mut cipher = AesGcm::new(key);
        let mut plaintext = cipher.decrypt(nonce, &self.payload.payload.as_slice())?;
        self.payload.payload = plaintext;
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


