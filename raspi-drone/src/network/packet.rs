use std::error::Error;
use std::fmt::{Display, Formatter};
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::{Key, KeySizeUser};

/// max num of bytes in a LoRa packetdep
pub const MAX_PACKET_SIZE: usize = 255;
pub const HEADER_SIZE: usize = 7;
/// MAX_PACKET_SIZE - header - payload crc size
pub const MAX_PAYLOAD_SIZE: usize = MAX_PACKET_SIZE - HEADER_SIZE - 2;

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




pub trait PacketHeader {
    fn calculate_header_crc(&mut self);
    fn check_header_crc(&self) -> bool;
}

pub trait PacketPayload {
    fn calculate_payload_crc(&mut self) -> Result<(), Box<dyn std::error::Error>>;
    fn check_payload_crc(&self) -> Result<bool, Box<dyn std::error::Error>>;
}


pub trait PacketEncrypt<KeySize>
where KeySize: KeySizeUser
{
    fn encrypt_payload(&mut self, key: Key<KeySize>) -> Result<(), Box<dyn std::error::Error>>;
    fn decrypt_payload(&mut self, key: Key<KeySize>) -> Result<(), Box<dyn std::error::Error>>;
}

pub trait Packet {

}


#[derive(Debug, Default, Copy, Clone, PartialEq, Eq)]
pub struct LoRaPacketHeader {
    pub source_addr: u8,
    pub dest_addr: u8,
    pub message_packet_num: u8,
    pub packet_num: u8,
    pub payload_size: u8,
    pub header_crc: u16
}

impl LoRaPacketHeader {
    pub fn new(source_addr: u8,
               dest_addr: u8,
               message_packet_num: u8,
               packet_num: u8,
               payload_size: u8,
               header_crc: u16) -> Result<Self, PacketSizeError> {

        if payload_size as usize > MAX_PAYLOAD_SIZE {
            Err(PacketSizeError::new(payload_size as usize))
        } else {
            Ok(LoRaPacketHeader {
                source_addr,
                dest_addr,
                message_packet_num,
                packet_num,
                payload_size,
                header_crc,
            })
        }
    }
}

impl PacketHeader for LoRaPacketHeader {
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
                dest_addr: value[1],
                message_packet_num: value[2],
                packet_num: value[3],
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
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
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
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
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
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
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
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
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

impl LoRaPacketPayload {
    pub fn new(payload_crc: u16, payload: Vec<u8>) -> Result<Self, PacketSizeError> {
        if payload.len() > MAX_PAYLOAD_SIZE {
            Err(PacketSizeError::new(payload.len()))
        } else {
            Ok(LoRaPacketPayload {
                payload_crc,
                payload: payload.clone(),
            })
        }
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
    header: LoRaPacketHeader,
    payload: LoRaPacketPayload
}

impl Packet for LoRaPacket {}

impl LoRaPacket {
    pub fn new(header: LoRaPacketHeader, payload: LoRaPacketPayload) -> Self {
        LoRaPacket {
            header,
            payload,
        }
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
