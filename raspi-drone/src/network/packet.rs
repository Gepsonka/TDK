use aes_gcm::aead::consts::U12;
use aes_gcm::aead::{Aead, AeadMut};
use aes_gcm::{Aes128Gcm, Key, KeyInit, Nonce, TagSize};
use std::error::Error;
use std::fmt::{Display, Formatter};

use super::lora::LoRa;

/// max num of bytes in a LoRa packet
pub const MAX_PACKET_SIZE: usize = 255;
pub const HEADER_SIZE: usize = 7;

/// MAX_PACKET_SIZE - header - payload crc size
pub const MAX_PAYLOAD_SIZE: usize = MAX_PACKET_SIZE - HEADER_SIZE - 2;

/// = packet max size (MAX_PAYLOAD_SIZE) - nonce size (12)
pub const MAX_MESSAGE_SLICE_SIZE: usize = MAX_PAYLOAD_SIZE - 12;

pub const AES_GCM_128_TAG_SIZE: u8 = 16;

pub const AES_GCM_128_NONCE_SIZE: u8 = 12;

/// MAX_PAYLOAD_SIZE - NONCE_SIZE - TAG_SIZE
pub const MAX_RAW_MESSAGE_SIZE_AES_GCM_128: usize =
    MAX_PAYLOAD_SIZE - AES_GCM_128_NONCE_SIZE as usize - AES_GCM_128_TAG_SIZE as usize;

#[derive(Debug)]
pub struct PacketPayloadSizeError {
    payload_size: u8,
}

impl PacketPayloadSizeError {
    fn new(payload_size: u8) -> Self {
        PacketPayloadSizeError { payload_size }
    }
}

impl Display for PacketPayloadSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Payload size must me smaller than {}! ({})",
            MAX_PAYLOAD_SIZE, self.payload_size
        )
    }
}

impl Error for PacketPayloadSizeError {}

#[derive(Debug)]
pub struct PacketSizeError {
    packet_size: usize,
}

impl PacketSizeError {
    pub fn new(packet_size: usize) -> Self {
        PacketSizeError { packet_size }
    }
}

impl Display for PacketSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Packet size must me smaller than {}! ({})",
            MAX_PACKET_SIZE, self.packet_size
        )
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
    fn new_from_slice(slice: [u8; HEADER_SIZE]) -> Result<Self, Box<dyn std::error::Error>>
    where
        Self: Sized;
}

pub trait PacketPayloadInit {
    fn new(payload: Vec<u8>) -> Result<Self, PacketPayloadSizeError>
    where
        Self: Sized;
    fn new_from_slice(slice: &[u8]) -> Result<Self, Box<dyn std::error::Error>>
    where
        Self: Sized;
}

pub trait PacketInit {
    type Header;
    type Payload;

    fn new(header: Self::Header, payload: Self::Payload) -> Self
    where
        Self: Sized;
    fn new_from_slice(slice: &[u8]) -> Result<Self, Box<dyn std::error::Error>>
    where
        Self: Sized;
}

/// Every network communication is done via packets.
/// This trait defines the basic functionality of a packet header
/// required for the network communication of drones.
/// Each packet must have a header and payload CRC to ensure data integrity,
/// since every packet is sent through a wireless medium.
pub trait PacketHeaderCrcCalculator {
    fn calculate_header_crc(&mut self);
    fn check_header_crc(&self) -> bool;
}


pub trait PacketPayloadCrcCalculator {
    fn calculate_payload_crc(&mut self) -> Result<(), Box<dyn std::error::Error>>;
    fn check_payload_crc(&self) -> Result<bool, Box<dyn std::error::Error>>;
}


/// Every packet must implement this trait,
/// since after successful authentication 
/// every packet must be encrypted.
pub trait PacketEncrypt<AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    fn encrypt(
        &mut self,
        key: &Key<AesGcm>,
        nonce: &Nonce<AesGcm::NonceSize>,
    ) -> Result<(), Box<dyn Error>>;
}


pub trait PacketDecrypt<AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    fn decrypt(&mut self, key: &Key<AesGcm>) -> Result<(), Box<dyn std::error::Error>>;
}


/// Since the protocol is mostly standardised
/// the packet fields are the same/almost same for every packet.
/// This trait defines the basic fields and its types of a packet.
pub trait PacketFields {
    type AddressSizeType;
    /// For specifying the max paylaod count size in a message
    type PayloadCountType;
    type PayloadSizeType;
    type CrcSizeType;

    type Payload;

    fn get_source_address(&self) -> Self::AddressSizeType;

    fn get_destination_address(&self) -> Self::AddressSizeType;

    fn get_message_packet_num(&self) -> Self::PayloadCountType;

    fn get_total_number_of_packets(&self) -> Self::PayloadCountType;

    fn get_payload_size(&self) -> Self::PayloadSizeType;

    fn get_header_crc(&self) -> Self::CrcSizeType;

    fn get_payload_crc(&self) -> Self::CrcSizeType;

    fn get_payload(&self) -> Self::Payload;

    fn get_payload_ref(&self) -> &Self::Payload;

    fn get_payload_mut(&mut self) -> &mut Self::Payload;
}


/// Contains all the packet relates sizes.
/// For example: max packet size, max payload when encrypted, etc.
/// Every packet implementation must implement this trait.
pub trait PacketSize
{
    const MAX_PACKET_SIZE: usize;
    const HEADER_SIZE: usize;
    const MAX_PAYLOAD_SIZE: usize;
    const TAG_SIZE: usize;
    const NONCE_SIZE: usize;

    /// Returns the max size of the raw message that can be encrypted
    fn get_max_encrypted_raw_size() -> usize {
        Self::MAX_PAYLOAD_SIZE - Self::TAG_SIZE - Self::NONCE_SIZE
    }
}



/// Header of the LoRa packet
/// Contains the source and destination address, the packet number in the message,
/// the total number of packets in the message and the payload size.
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

impl PacketHeaderCrcCalculator for LoRaPacketHeader {
    /// Calculates the header CRC and sets it to the header_crc field
    fn calculate_header_crc(&mut self) {
        let header: Vec<u8> = self.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        self.header_crc = X25.checksum(&header.as_slice()[0..=4]);
    }

    /// Checks if the header CRC is correct
    fn check_header_crc(&self) -> bool {
        let header: Vec<u8> = self.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        X25.checksum(&header.as_slice()[0..=4]) == self.header_crc
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
                header_crc: ((value[5] as u16) << 8) | value[6] as u16,
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
            (self.header_crc & 0xFF) as u8,
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
            (self.header_crc & 0xFF) as u8,
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
            (self.header_crc & 0xFF) as u8,
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
            (self.header_crc & 0xFF) as u8,
        ]
    }
}

/// A packet is the basic unit of communication in the network.
/// It contains a header and a payload.
/// The header contains the source and destination address, the packet number in the message,
/// the total number of packets in the message and the payload size.
/// The payload contains the actual data.
/// The packet is encrypted using AES-GCM-128.
/// The nonce is the last 12 bytes of the payload.
/// It is an implementation for a custom LoRa mesh network.
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

    /// Creates a new payload from the slice.
    /// Does not calculate the CRC! Automatically sets it to 0.
    ///
    /// ## Arguments
    ///
    /// * `slice` - Payload slice
    /// 
    /// ## Returns
    /// 
    /// * `Result<LoRaPacketPayload, Box<dyn Error>>` - If the payload size is bigger than the max payload size
    /// returns an error, otherwise returns the payload.
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
        let mut payload_vec: Vec<u8> = vec![
            ((self.payload_crc >> 8) & 0xFF) as u8,
            (self.payload_crc & 0xFF) as u8,
        ];
        payload_vec.append(&mut self.payload);

        payload_vec
    }
}

#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct LoRaPacket {
    pub header: LoRaPacketHeader,
    pub payload: LoRaPacketPayload,
}

impl PacketInit for LoRaPacket {
    type Header = LoRaPacketHeader;
    type Payload = LoRaPacketPayload;

    fn new(header: Self::Header, payload: Self::Payload) -> Self {
        LoRaPacket { header, payload }
    }

    fn new_from_slice(slice: &[u8]) -> Result<Self, Box<dyn Error>> {
        if slice.len() > MAX_PACKET_SIZE {
            return Err(Box::new(PacketSizeError::new(slice.len())));
        }

        let header = LoRaPacketHeader::try_from(slice[0..=6].to_vec())?;
        let payload = LoRaPacketPayload::try_from(slice[7..slice.len()].to_vec())?;

        Ok(LoRaPacket { header, payload })
    }
}

impl PacketFields for LoRaPacket {
    type AddressSizeType = u8;
    type PayloadCountType = u8;
    type PayloadSizeType = u8;
    type CrcSizeType = u16;
    type Payload = Vec<u8>;

    fn get_source_address(&self) -> Self::AddressSizeType {
        self.header.source_addr
    }

    fn get_destination_address(&self) -> Self::AddressSizeType {
        self.header.destination_addr
    }

    fn get_message_packet_num(&self) -> Self::PayloadCountType {
        self.header.message_packet_num
    }

    fn get_total_number_of_packets(&self) -> Self::PayloadCountType {
        self.header.total_number_of_packets
    }

    fn get_payload_size(&self) -> Self::PayloadSizeType {
        self.header.payload_size
    }

    fn get_header_crc(&self) -> Self::CrcSizeType {
        self.header.header_crc
    }

    fn get_payload_crc(&self) -> Self::CrcSizeType {
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


impl PacketSize for LoRaPacket {
    const MAX_PACKET_SIZE: usize = 255;
    const HEADER_SIZE: usize = 7;
    const MAX_PAYLOAD_SIZE: usize = Self::MAX_PACKET_SIZE - Self::HEADER_SIZE - 2;
    // change it to the aes_gcm lib ones (searching for it....)
    const NONCE_SIZE: usize = 12;
    const TAG_SIZE: usize = 16;
}


impl PacketEncrypt<Aes128Gcm> for LoRaPacket {
    /// Encrypts the payload of the packet.
    /// ## Arguments:
    /// 
    /// * `key` - The key used for encryption
    /// * `nonce` - The nonce used for encryption
    /// 
    fn encrypt(&mut self, key: &Key<Aes128Gcm>, nonce: &Nonce<U12>) -> Result<(), Box<dyn Error>> {
        let packet_content = self.payload.payload.clone();
        let cipher = Aes128Gcm::new(key);
        let ciphertext = cipher
            .encrypt(nonce, self.payload.payload.as_slice())
            .map_err(|_e| PacketEncryptionError {})?;

        self.payload.payload = ciphertext;
        self.payload
            .payload
            .append(nonce.as_slice().to_vec().as_mut());

        if self.payload.payload.len() > MAX_PAYLOAD_SIZE {
            self.payload.payload = packet_content;
            return Err(Box::new(PacketSizeError::new(self.payload.payload.len())));
        }

        Ok(())
    }
}

impl PacketDecrypt<Aes128Gcm> for LoRaPacket {
    /// Decrypts the payload of the packet.
    /// Packet includes both the tag and the nonce
    /// ## Arguments:
    /// 
    /// * `key` - The key used for decryption
    /// 
    /// ## Returns:
    /// 
    /// * `Result<(), Box<dyn Error>>` - If the decryption was successful returns nothing
    /// otherwise returns an error (this happens when the tag is not correct).
    /// 
    fn decrypt(&mut self, key: &Key<Aes128Gcm>) -> Result<(), Box<dyn Error>> {
        let cipher = Aes128Gcm::new(key);
        let packet_content = self.payload.payload.clone();
        let mut ciphertext = self.payload.payload.clone();

        let mut nonce_slice = [0u8; 12];
        nonce_slice
            .copy_from_slice(&packet_content[packet_content.len() - 12..packet_content.len()]);
        let nonce = Nonce::from_slice(&nonce_slice);

        ciphertext.truncate(ciphertext.len() - 12);

        self.payload.payload = cipher
            .decrypt(&nonce, ciphertext.as_slice())
            .map_err(|_e| PacketDecryptionError {})?;

        Ok(())
    }
}

impl PacketPayloadCrcCalculator for LoRaPacketPayload {
    /// Calculates the payload CRC and sets it to the payload_crc field
    fn calculate_payload_crc(&mut self) -> Result<(), Box<dyn Error>> {
        let payload: Vec<u8> = self.payload.clone();
        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        self.payload_crc = X25.checksum(&payload);

        Ok(())
    }

    /// Checks if the payload CRC is correct
    fn check_payload_crc(&self) -> Result<bool, Box<dyn Error>> {
        let payload: Vec<u8> = self.payload.clone();
        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        Ok(X25.checksum(&payload) == self.payload_crc)
    }
}

impl PacketHeaderCrcCalculator for LoRaPacket {
    /// Calculates the header CRC and sets it to the header_crc field
    fn calculate_header_crc(&mut self) {
        let header: Vec<u8> = self.header.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        self.header.header_crc = X25.checksum(&header.as_slice()[0..=4]);
    }

    /// Checks if the header CRC is correct
    fn check_header_crc(&self) -> bool {
        let header: Vec<u8> = self.header.into();

        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_KERMIT);
        X25.checksum(&header.as_slice()[0..=4]) == self.header.header_crc
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

        Ok(LoRaPacket { header, payload })
    }
}
