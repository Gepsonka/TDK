use crate::network::packet::error_types::{PacketPayloadSizeError, PacketSizeBigError, PacketSizeError, CRCError};
use crate::network::packet::traits::{PacketHeader, PacketPayload, Packet};
use std::convert::From;
use std::error::Error;


/// max num of bytes in a LoRa packets
const MAX_PACKET_SIZE: usize = 255;
const HEADER_SIZE: usize = 7;
/// MAX_PACKET_SIZE - header - payload crc size
const MAX_PAYLOAD_SIZE: usize = MAX_PACKET_SIZE - HEADER_SIZE - 2;




#[derive(Debug, Default, Copy, Clone)]
struct LoRaPacketHeader {
    source_addr: u8,
    dest_addr: u8,
    message_packet_num: u8,
    packet_num: u8,
    payload_size: u8,
    header_crc: u16
}

impl LoRaPacketHeader {
    pub fn new(source_addr: u8,
               dest_addr: u8,
               message_packet_num: u8,
               packet_num: u8,
               payload_size: u8,
               header_crc: u16) -> Result<Self, PacketSizeBigError> {

        if payload_size as usize > MAX_PAYLOAD_SIZE {
            Err(PacketSizeBigError::new(payload_size))
        }

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

impl PacketHeader for LoRaPacketHeader {
    fn calculate_header_crc(&mut self: LoRaPacketHeader) {
        const X25: crc::Crc<u16> = crc::Crc::<u16>::new(&crc::CRC_16_IBM_SDLC);
        self.header_crc = X25.checksum(self.into()[0..5]);
    }

    fn check_header_crc(&self: LoRaPacketHeader) -> bool {
        X25.checksum(self.into()[0..5]) == self.header_crc
    }
}


impl From<[u8; 7]> for LoRaPacketHeader {
    fn from(value: [u8; 7]) -> Result<LoRaPacketHeader, PacketSizeBigError> {
        Ok(LoRaPacketHeader::new(
            value[0],
            value[1],
            value[2],
            value[3],
            value[4],
            (value[5] << 8 | value[6]) as u16
        )?)
    }
}

impl Into<[u8; 7]> for LoRaPacketHeader {
    fn into(&self) -> [u8; 7] {
        [
            self.source_addr,
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
            self.payload_size,
            (self.header_crc >> 8) & 0xFF as u8,
            self.header_crc & 0xFF as u8
        ]
    }
}

impl Into<Vec<u8>> for LoRaPacketHeader {
    fn into(self) -> Vec<u8> {
        [
            self.source_addr,
            self.dest_addr,
            self.message_packet_num,
            self.packet_num,
            self.payload_size,
            (self.header_crc >> 8) & 0xFF as u8,
            self.header_crc & 0xFF as u8
        ].to_vec()
    }
}

#[derive(Debug, Default, Clone, Copy)]
struct LoRaPacketPayload {
    payload_crc: u16,
    payload: Vec<u8>,

}

impl LoRaPacketPayload {
    pub fn new(payload_crc: u16, payload: Vec<u8>) -> Result<Self, PacketPayloadSizeError> {
        if payload.len() > MAX_PAYLOAD_SIZE {
            Err(PacketPayloadSizeError)
        }

        Ok(LoRaPacketPayload {
            payload_crc,
            payload: payload.clone(),
        })
    }
}

impl From<Vec<u8>> for LoRaPacketPayload {
    /// First two bytes of @value must be the crc then comes the payload
    fn from(value: Vec<u8>) -> Result<Self, PacketPayloadSizeError> {
        if value.len() > MAX_PACKET_SIZE - HEADER_SIZE {
            Err(PacketPayloadSizeError)
        }
        
        Ok(LoRaPacketPayload{
            payload_crc: ((value[0] << 8) | value[1]) as u16 ,
            payload: value[2..value.len()].to_vec(),
        })
    }
}


impl Into<Vec<u8>> for LoRaPacketPayload {
    fn into(&mut self) -> Vec<u8> {
        let mut payload_vec: Vec<u8> = vec![(self.header_crc >> 8) & 0xFF as u8,
                               self.header_crc & 0xFF as u8];
        payload_vec.append(&mut self.payload);

        payload_vec

    }
}

pub struct LoRaPacket {
    header: LoRaPacketHeader,
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


impl Into<Vec<u8>> for LoRaPacket {
    fn into(self) -> Vec<u8> {
        todo!()
    }
}

impl From<Vec<u8>> for LoRaPacket {
    fn from(value: Vec<u8>) -> Self {
        todo!()
    }
}




impl LoRaPakcet {
    pub fn new(packet_header: LoRaPacketHeader, packet_payload: LoRaPacketPayload) -> Result<LoRaPakcet, ()> {

    }
}