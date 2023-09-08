use super::lora::LoRa;
use super::packet::{LoRaPacketHeader, PacketHeaderInit, PacketInit, LoRaPacketPayload, PacketPayloadInit, PacketHeaderCrcCalculator};
use crate::network::device_status::DeviceStatus;
use crate::network::device_type::DeviceType;
use crate::network::packet::{LoRaPacket, PacketDecrypt, MAX_MESSAGE_SLICE_SIZE, MAX_PACKET_SIZE};
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use aes_gcm::aead::{AeadMut, KeyInit};
use aes_gcm::{AeadCore, Aes128Gcm, Key, KeySizeUser, Nonce};
use std::borrow::Borrow;
use std::collections::BTreeMap;
use std::hash::Hash;
use std::time::{Duration, SystemTime};

#[derive(Debug, Clone, Hash, Eq, PartialOrd)]
pub struct InitializationVector<NonceSize>
where
    NonceSize: ArrayLength<u8>,
{
    iv: Nonce<NonceSize>,
    expiration_duration: Duration,
    creation_time: SystemTime,
}

impl<NonceSize> InitializationVector<NonceSize>
where
    NonceSize: ArrayLength<u8>,
{
    pub fn new(iv: Nonce<NonceSize>, expiration: Duration, creation_time: SystemTime) -> Self {
        let nonce = Aes128Gcm::generate_nonce(&mut OsRng);
        InitializationVector {
            iv,
            expiration_duration: expiration,
            creation_time,
        }
    }

    fn duration_since_creation(&self) -> Duration {
        Duration::from_millis(self.creation_time.elapsed().unwrap().as_millis() as u64)
    }
}

impl<NonceSize> PartialEq<Self> for InitializationVector<NonceSize>
where
    NonceSize: ArrayLength<u8>,
{
    fn eq(&self, other: &Self) -> bool {
        self.iv == other.iv
    }
}

impl<NonceSize> Ord for InitializationVector<NonceSize>
where
    NonceSize: ArrayLength<u8> + Eq + PartialOrd,
{
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.iv.as_slice().cmp(&other.iv.as_slice())
    }
}

#[derive(Debug, Clone, Hash)]
pub struct InitializationVectorContainer<NonceSize>
where
    NonceSize: ArrayLength<u8>,
{
    initialization_vectors: BTreeMap<Nonce<NonceSize>, InitializationVector<NonceSize>>,
}

impl<NonceSize> InitializationVectorContainer<NonceSize>
where
    NonceSize: ArrayLength<u8>,
{
    pub fn new() -> Self {
        InitializationVectorContainer {
            initialization_vectors: BTreeMap::new(),
        }
    }
}

pub trait ArpRegistryInit {
    fn new(address: u8, device_status: DeviceStatus) -> Self;
}

pub trait PacketDecryptor {
    type PacketType;

    fn decrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), ()>;
    fn decrypt_packets(&mut self, packets: &mut Vec<Self::PacketType>) -> Result<(), ()>;
}

pub trait PacketEncryptor {
    type PacketType;

    fn encrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), ()>;
    fn encrypt_packets(&mut self, packets: &mut Vec<Self::PacketType>) -> Result<(), ()>;
}

pub trait MessageAssembler {
    /// Assembles a message from a vector of packets
    fn assemble_message_from_packets(&mut self) -> Result<(), ()>;
}

pub trait MessageDisassembler {
    fn disassemble_message_into_packets(&mut self) -> Result<(), ()>;
}

pub trait PacketHandler {
    fn calc_crc_and_encrypt_tx_packets(&mut self);
    fn check_crc_and_decrypt_packets(&mut self);
}


#[derive(Debug, Clone)]
pub struct ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    pub address: Option<u8>,
    pub device_type: Option<DeviceType>,
    pub device_status: DeviceStatus,
    pub secret_key: Option<Key<AesGcm>>,
    pub packet_rx_vec: Vec<PacketT>,
    packet_tx_vec: Vec<PacketT>,
    pub rx_message: Vec<u8>,
    tx_message: Vec<u8>,
    faulty_packets: Vec<u8>,
    used_ivs: InitializationVectorContainer<AesGcm::NonceSize>,
    iv_expiration_duration: Option<Duration>,
}

impl<AesGcm> ArpRegistryInit for ArpRegistry<LoRaPacket, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    fn new(address: Option<u8>, device_status: DeviceStatus) -> Self {
        ArpRegistry {
            address,
            device_type: None,
            device_status,
            secret_key: None,
            packet_rx_vec: Vec::new(),
            packet_tx_vec: Vec::new(),
            rx_message: Vec::new(),
            tx_message: Vec::new(),
            faulty_packets: Vec::new(),
            used_ivs: InitializationVectorContainer::new(),
            iv_expiration_duration: Some(Duration::from_secs(5)),
        }
    }
}

impl<AesGcm> PacketDecryptor for ArpRegistry<LoRaPacket, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    type PacketType = LoRaPacket;

    fn decrypt_packet(&mut self, &mut packet: LoRaPacket) -> Result<(), ()> {
        if let Some(key) = self.secret_key {
            packet.decrypt(&key)?;
        }

        Ok(())
    }

    fn decrypt_packets(&mut self, &mut packets: Vec<LoRaPacket>) -> Result<(), ()> {
        if let Some(key) = self.secret_key {
            for packet in packets {
                packet.decrypt(&key)?;
            }
        }

        Ok(())
    }
}

impl<AesGcm> PacketEncryptor for ArpRegistry<LoRaPacket, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
{
    type PacketType = LoRaPacket;

    fn encrypt_packet(&mut self, &mut packet: LoRaPacket) -> Result<(), ()> {
        if let Some(key) = self.secret_key {
            packet.encrypt(&key)?;
        }

        Ok(())
    }

    fn encrypt_packets(&mut self, &mut packets: Vec<LoRaPacket>) -> Result<(), ()> {
        if let Some(key) = self.secret_key {
            for packet in packets {
                packet.encrypt(&key)?;
            }
        }

        Ok(())
    }
}

impl<AesGcm> MessageAssembler for ArpRegistry<LoRaPacket, AesGcm>
where
    AesGcm: AeadMut,
{
    type PacketType = LoRaPacket;
    /// Assembles a message from a vector of packets
    /// All packets are assumed to be decrypted
    fn assemble_message_from_packets(&mut self) -> Result<(), ()> {
        self.rx_message.clear();

        for packet in self.packet_rx_vec.iter() {
            self.rx_message.extend_from_slice(&packet.payload.payload);
        }

        Ok(())
    }
}

impl<AesGcm> MessageDisassembler for ArpRegistry<LoRaPacket, AesGcm> 
where AesGcm: AeadMut {
    type PacketType = LoRaPacket;

    /// Disassmebles the message the device wants to send to the address.
    /// Does not calculate CRC nor encrypts the message. Must be done at a later
    /// step
    fn disassemble_message_into_packets(&mut self) -> Result<(), ()> {
        self.packet_tx_vec.clear();

        let packet_count: usize = self
            .tx_message
            .chunks(MAX_RAW_MESSAGE_SIZE_AES_GCM_128)
            .count();

        self.tx_message
            .chunks(MAX_RAW_MESSAGE_SIZE_AES_GCM_128)
            .enumerate()
            .for_each(|(index, item)| {
                // TODO: Change first element of the slice to the true address
                self.packet_tx_vec
                    .push(
                        LoRaPacket::new(
                            LoRaPacketHeader::new_from_slice([
                                0, // change it later
                                self.address,
                                index as u8,
                                packet_count as u8,
                                item.len() + AES_GCM_128_TAG_SIZE + AES_GCM_128_NONCE_SIZE,
                                0, // calculate it at a later stage
                            ]),
                        LoRaPacketPayload::new_from_slice(slice).unwrap()
                    )
                )
            });
        Ok(())
    }
}


impl PacketHandler for ArpRegistry<LoRaPacket, AesGcm>
where AesGcm: AeadMut {
    fn check_crc_and_decrypt_packets(&mut self) {
        self.faulty_packets.clear();
        for packet in self.packet_rx_vec.iter() {
            if !packet.header.check_header_crc() {
                self.faulty_packets.push(packet.clone())
            }

            // assume if the secret_key is None, then the packet is not encrypted
            if let Some(key) = self.secret_key {
                let packet_decrpyt_result = packet.decrypt(&key);
                match packet_decrpyt_result {
                    Err(_) => self.faulty_packets.push(packet.clone()),
                    Ok(_) => {}
                }
            }
            
        }   
    }

    fn calc_crc_and_encrypt_tx_packets(&mut self) {
        for 
        
    }
}