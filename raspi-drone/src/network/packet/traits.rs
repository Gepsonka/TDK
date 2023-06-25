use crate::network::error_types::{PacketPayloadSizeError, PacketSizeError};

pub trait PacketHeader {
    fn calculate_header_crc(&mut self: dyn PacketHeader);
    fn check_header_crc(&mut self: dyn PacketHeader) -> bool;
}

pub trait PacketPayload {
    fn calculate_payload_crc(&mut self: dyn PacketPayload) -> Result<(), Box(dyn std::error::Error)>;
    fn check_payload_crc(&self: dyn PacketPayload) -> Result<bool, Box(dyn std::error::Error)>;
}


pub trait Packet {
    
}