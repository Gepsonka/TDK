use super::lora::LoRa;
use super::packet::{
    self, LoRaPacket, LoRaPacketHeader, LoRaPacketPayload, PacketDecrypt, PacketEncrypt,
    PacketFields, PacketHeaderCrcCalculator, PacketHeaderInit, PacketInit,
    PacketPayloadCrcCalculator, PacketPayloadInit, AES_GCM_128_NONCE_SIZE, AES_GCM_128_TAG_SIZE,
    MAX_MESSAGE_SLICE_SIZE, MAX_PACKET_SIZE, MAX_PAYLOAD_SIZE, MAX_RAW_MESSAGE_SIZE_AES_GCM_128,
    PacketSize
};
use crate::device_self::DeviceSelf;
use crate::network::device_status::DeviceStatus;
use crate::network::device_type::DeviceType;
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use aes_gcm::aead::{AeadMut, KeyInit};
use aes_gcm::{AeadCore, Aes128Gcm, Key, KeySizeUser, Nonce};
use log::error;
use std::borrow::Borrow;
use std::collections::BTreeMap;
use std::error::Error;
use std::fmt::Debug;
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
    pub initialization_vectors: BTreeMap<Nonce<NonceSize>, InitializationVector<NonceSize>>,
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
    fn new(address: Option<u8>, device_status: DeviceStatus) -> Self;
}

pub trait PacketDecryptor {
    type PacketType;

    fn decrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), Box<dyn Error>>;
    fn decrypt_packets(&mut self, packets: &mut Vec<Self::PacketType>);
}

pub trait PacketEncryptor {
    type PacketType;

    fn encrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), Box<dyn Error>>;
    fn encrypt_packets(
        &mut self,
        packets: &mut Vec<Self::PacketType>,
    ) -> Result<(), Box<dyn Error>>;
}

pub trait MessageAssembler {
    /// Assembles a message from a vector of packets
    fn assemble_message_from_packets(&mut self);
}

pub trait MessageDisassembler
where 
    Self: RegistryFields
{
    fn disassemble_message_into_packets(&mut self, device_self: &DeviceSelf<Self::AddressSize>) -> Result<(), ()>;
}

pub trait PacketHandler {
    fn finalise_tx_packets(&mut self);
    fn finalise_rx_packets(&mut self);
}

pub trait RegistryFields {
    type AddressSize;
    type AesGcmT;

    fn get_address(&self) -> Option<Self::AddressSize>;
    fn get_secret_key(&self) -> Option<Key<Self::AesGcmT>>
    where
        Self::AesGcmT: AeadMut + KeyInit;
}

/// This will store every network related data of a device
/// from secrets to packets. Message handling and error handling for a network entity will
/// happen here.
#[derive(Debug, Clone)]
pub struct ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketFields,
{
    pub address: Option<u8>,
    pub device_type: Option<DeviceType>,
    pub device_status: DeviceStatus,
    pub secret_key: Option<Key<AesGcm>>,
    pub packet_rx_vec: Vec<PacketT>,
    packet_tx_vec: Vec<PacketT>,
    pub rx_message: Vec<u8>,
    tx_message: Vec<u8>,
    faulty_packets: Vec<PacketT::PayloadCountType>,
    used_ivs: InitializationVectorContainer<AesGcm::NonceSize>,
    iv_expiration_duration: Option<Duration>,
}

impl<PacketT, AesGcm> ArpRegistryInit for ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketFields
        + PacketInit
        + PacketDecrypt<Aes128Gcm>
        + PacketEncrypt<Aes128Gcm>
        + TryFrom<Vec<u8>>,
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
            iv_expiration_duration: Some(Duration::from_secs(30)),
        }
    }
}

impl<PacketT, AesGcm> RegistryFields for ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketFields,
{
    type AddressSize = u8;
    type AesGcmT = AesGcm;

    fn get_address(&self) -> Option<Self::AddressSize> {
        self.address
    }

    fn get_secret_key(&self) -> Option<Key<Self::AesGcmT>> {
        self.secret_key.clone()
    }
}

impl<PacketT> PacketDecryptor for ArpRegistry<PacketT, Aes128Gcm>
where
    PacketT: PacketFields + PacketDecrypt<Aes128Gcm>,
{
    type PacketType = PacketT;

    fn decrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), Box<dyn Error>> {
        if let Some(key) = self.secret_key {
            packet.decrypt(&key)?;
        }

        Ok(())
    }

    fn decrypt_packets(&mut self, packets: &mut Vec<Self::PacketType>) {
        if let Some(key) = self.secret_key {
            for packet in packets {
                let decr_res = packet.decrypt(&key);
                match decr_res {
                    Err(_) => self.faulty_packets.push(packet.get_message_packet_num()),
                    Ok(_) => {}
                };
            }
        }
    }
}

impl<PacketT> PacketEncryptor for ArpRegistry<PacketT, Aes128Gcm>
where
    PacketT: PacketFields + PacketEncrypt<Aes128Gcm>,
{
    type PacketType = PacketT;

    fn encrypt_packet(&mut self, packet: &mut Self::PacketType) -> Result<(), Box<dyn Error>> {
        if let Some(key) = self.secret_key {
            let mut nonce;
            loop {
                nonce = Aes128Gcm::generate_nonce(&mut OsRng);
                if !self.used_ivs.initialization_vectors.contains_key(&nonce) {
                    break;
                }
            }
            packet.encrypt(&key, &nonce)?;
        }

        Ok(())
    }

    fn encrypt_packets(
        &mut self,
        packets: &mut Vec<Self::PacketType>,
    ) -> Result<(), Box<dyn Error>> {
        if let Some(key) = self.secret_key {
            for packet in packets {
                let mut nonce;
                loop {
                    nonce = Aes128Gcm::generate_nonce(&mut OsRng);
                    if !self.used_ivs.initialization_vectors.contains_key(&nonce) {
                        break;
                    }
                }
                packet.encrypt(&key, &nonce)?;
            }
        }

        Ok(())
    }
}

impl<PacketT, AesGcm> MessageAssembler for ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketFields<Payload = Vec<u8>>
{
    /// Assembles a message from a vector of packets
    /// All packets are assumed to be decrypted
    fn assemble_message_from_packets(&mut self) {
        self.rx_message.clear();

        for packet in self.packet_rx_vec.iter() {
            self.rx_message.extend_from_slice(packet.get_payload_ref());
        }

    }
}

impl<PacketT, AesGcm> MessageDisassembler for ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketFields + TryFrom<Vec<u8>> + PacketSize + Debug
{

    /// Disassmebles the message the device wants to send to the address.
    /// Does not calculate CRC nor encrypts the message. Must be done at a later
    /// step. Fails if the LoRaPacketPayload creation fails.
    fn disassemble_message_into_packets(&mut self, device_self: &DeviceSelf<u8>) -> Result<(), ()> {
        self.packet_tx_vec.clear();
        let payload_size;
        if let Some(key) = &self.secret_key {
            payload_size = PacketT::get_max_encrypted_raw_size();
        } else {
            payload_size = PacketT::MAX_PAYLOAD_SIZE;
        }

        let packet_count: usize = self.tx_message.chunks(payload_size).count();

        self.tx_message
            .chunks(payload_size)
            .enumerate()
            .for_each(|(index, item)| {
                // TODO: Change first element of the slice to the true address

                let mut packet_size = if let Some(sec_key) = &self.secret_key {
                    item.len() + PacketT::TAG_SIZE + PacketT::NONCE_SIZE
                } else {
                    item.len()
                };

                let mut self_addr = if let Some(self_addr) = device_self.address {
                    self_addr
                } else {
                    0
                };

                let packet_res = PacketT::try_from(vec![
                    self_addr, // change it later
                    self.address.unwrap(),
                    index as u8,
                    packet_count as u8,
                    packet_size as u8,
                    0, // calculate header crc at a later stage
                ]);

                match packet_res {
                    Ok(mut packet) => {
                        self.packet_tx_vec.push(packet);
                    }
                    Err(_) => {
                        error!(
                            "Message disassemble failed!\n Address: {}\nMessage: {:?}\nChunk index: {}",
                            self.address.unwrap(),
                            self.tx_message,
                            index
                        );
                    }
                }

            });
        Ok(())
    }
}

impl <PacketT, AesGcm> PacketHandler for ArpRegistry<PacketT, AesGcm>
where
    AesGcm: AeadMut + KeyInit,
    PacketT: PacketDecrypt<Aes128Gcm> + PacketPayloadCrcCalculator + PacketHeaderCrcCalculator
{
    /// Check CRCs and decrpyts the packets if needed.
    /// Prepares packets for message assembly
    fn finalise_rx_packets(&mut self) {
        self.faulty_packets.clear();
        for packet in self.packet_rx_vec.iter() {
            if !packet.check_payload_crc() && !packet.header.check_header_crc() {
                self.faulty_packets.push(packet.header.message_packet_num);
            } else {
                // assume if the secret_key is None, then the packet is not encrypted
                if let Some(key) = self.secret_key {
                    let packet_decrpyt_result = packet.decrypt(&key);
                    match packet_decrpyt_result {
                        Err(_) => self.faulty_packets.push(packet.header.message_packet_num),
                        Ok(_) => {}
                    }
                }
            }
        }
    }

    /// Calculates CRCs and encrypts the packets if needed.
    /// Prepares packets for transmission
    fn finalise_tx_packets(&mut self) {
        for packet in self.packet_tx_vec.iter_mut() {
            packet.header.calculate_header_crc();
            if let Some(key) = self.secret_key {
                let nonce;
                loop {
                    nonce = Aes128Gcm::generate_nonce(&mut OsRng);
                    if !self.used_ivs.initialization_vectors.contains_key(&nonce) {
                        break;
                    }

                    if let Some(iv_expr_dur) = self.iv_expiration_duration {
                        self.used_ivs.initialization_vectors.insert(nonce, InitializationVector::new(nonce, iv_expr_dur, SystemTime::now()))

                    } else {
                        // Default duration will be 30 secs
                        self.used_ivs.initialization_vectors.insert(nonce, InitializationVector::new(nonce, Duration::from_secs(30), SystemTime::now()))

                    }
                }

                packet.encrypt(&key, &nonce).unwrap(); // TODO: Handle this error later
            }

            packet.payload.calculate_payload_crc();
        }
    }
}
