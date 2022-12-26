


pub enum LoRaStatus {
    Online,
    Offline
}


impl LoRaStatus {
    pub fn as_str(&self) -> &str {
        match self {
            LoRaStatus::Online => "Online",
            LoRaStatus::Offline => "Offline"
        }
    }
}


pub struct LoRa {
    status: LoRaStatus
}


impl LoRa {
    pub fn new() -> LoRa {
        LoRa { status: LoRaStatus::Offline }
    }
}