use std::collections::{BTreeMap, LinkedList};
use std::hash::Hash;
use std::time::{Duration, SystemTime};
use aes_gcm::{AeadCore, Aes128Gcm, Key, KeySizeUser, Nonce};
use aes_gcm::aead::consts::U12;
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use crate::network::packet::{LoRaPacket};


pub trait InitializationVectorContainer
{
    type NonceSize: ArrayLength<u8>;

    fn delete_expired_ivs(&mut self);
    fn add_iv(&mut self, iv: &mut InitializationVector<Self::NonceSize>);
    fn check_if_iv_is_used(&self, iv: &InitializationVector<Self::NonceSize>) -> bool;
}




#[derive(Debug, Clone, Copy, PartialEq)]
pub enum DeviceStatus {
    KeyExchangeInitiated,
}

#[derive(Debug, Clone, Hash)]
pub struct InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
{
    iv: Nonce<NonceSize>,
    expiration_duration: Duration,
    creation_time: SystemTime
}


impl <NonceSize> InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
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

impl <NonceSize> PartialEq<Self> for InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
{
    fn eq(&self, other: &Self) -> bool {
        self.iv == other.iv
    }
}

#[derive(Debug, Clone, Hash)]
pub struct AESInitializationVectorContainer<NonceSize>
where NonceSize: ArrayLength<u8>
{
    initialization_vectors: BTreeMap<Nonce<NonceSize>, InitializationVector<NonceSize>>
}

impl <NonceSize> AESInitializationVectorContainer<NonceSize>
where NonceSize: ArrayLength<u8>
{
    pub fn new() -> Self {
        AESInitializationVectorContainer {
            initialization_vectors: BTreeMap::new(),
        }
    }
}

impl<NonceSize> InitializationVectorContainer for AESInitializationVectorContainer<NonceSize>
where NonceSize: ArrayLength<u8>
{
    type NonceSize = NonceSize;

    fn delete_expired_ivs(&mut self) {
        for (key, val) in self.initialization_vectors.clone().into_iter() {
            if val.creation_time.elapsed().unwrap() > val.expiration_duration {
                self.initialization_vectors.remove(&key);
            }
        }
    }

    fn add_iv(&mut self, iv: &mut InitializationVector<NonceSize>) {
        self.initialization_vectors.insert(iv.iv.clone(), iv.clone());
    }

    fn check_if_iv_is_used(&self, iv: &InitializationVector<NonceSize>) -> bool {
        self.initialization_vectors.contains_key(iv)
    }
}

#[derive(Debug, Clone)]
pub struct ArpRegistry<PacketT, KeySize, NonceSize>
    where KeySize: KeySizeUser,
    NonceSize: ArrayLength<u8>
{
    address: u8,
    pub(crate) device_status: DeviceStatus,
    pub(crate) secret_key: Option<Key<KeySize>>,
    packet_rx_vec: Vec<PacketT>,
    packet_tx_vec: Vec<PacketT>,
    pub(crate) rx_message: Vec<u8>,
    tx_message: Vec<u8>,
    faulty_packages: Vec<u8>,
    used_ivs: AESInitializationVectorContainer<NonceSize>,
    iv_expiration_duration: Option<Duration>
}


impl <KeySize, NonceSize> ArpRegistry<LoRaPacket, KeySize, NonceSize>
where KeySize: KeySizeUser,
NonceSize: ArrayLength<u8>
{
    pub fn new(address: u8, device_status: DeviceStatus) -> Self {
        ArpRegistry {
            address,
            device_status,
            secret_key: None,
            packet_rx_vec: Vec::new(),
            packet_tx_vec: Vec::new(),
            rx_message: Vec::new(),
            tx_message: Vec::new(),
            faulty_packages: Vec::new(),
            used_ivs: AESInitializationVectorContainer::new(),
            iv_expiration_duration: Some(Duration::from_secs(5))
        }
    }

    fn set_addr(&mut self, addr: u8) {
        self.address = addr;
    }

    fn set_secret_key(&mut self, secret_key: Key<KeySize>) {
        self.secret_key = Some(secret_key);
    }

    fn set_device_status(&mut self, device_status: DeviceStatus) {
        self.device_status = device_status;
    }

    fn set_iv_expiration_duration(&mut self, duration: Duration) {
        self.iv_expiration_duration = Some(duration);
    }
}





