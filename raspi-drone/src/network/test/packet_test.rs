#[cfg(test)]
mod tests {
    use crate::network::packet;
    use crate::network::packet::PacketHeader;

    #[test]
    #[should_panic]
    fn test_lora_packet_header() {
        let header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0xFF,
            0xFF,
            0xFFFF
        ).unwrap();

    }

    #[test]
    fn test_lora_packet_header_crc_calculation() {
        let mut header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        header.calculate_header_crc();

        assert_eq!(header.header_crc, 0xED9Bu16);
    }

    #[test]
    fn test_lora_packet_header_crc_checker() {
        let mut header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        header.calculate_header_crc();

        assert_eq!(header.check_header_crc(), true);
    }

    #[test]
    fn test_lora_packet_header_into_vec_u8() {
        let header = packet::LoRaPacketHeader::new(
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0xFFFF
        ).unwrap();

        let header_vec: Vec<u8> = header.into();

        let assert_header: Vec<u8> = vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF];

        assert_eq!(header_vec, assert_header);
    }

    #[test]
    #[should_panic]
    fn test_lora_packet_header_tryfrom_fail(){
        let header = packet::LoRaPacketHeader::try_from(vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF, 0x44]).unwrap();

    }


}
