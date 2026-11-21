#!/usr/bin/env bash
# ==============================================================================
# WAP610N Firmware Flasher Control Center
# Interactive CLI Dashboard untuk Flashing, Serial Terminal, & Diagnosa
# ==============================================================================

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="${BASE_DIR}/tools"
OUTPUT_DIR="${BASE_DIR}/firmware"

# Warna Output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

function show_banner() {
    clear
    echo -e "${CYAN}${BOLD}"
    echo "======================================================================"
    echo "       ⚡ CISCO LINKSYS WAP610N - FLASHER CONTROL CENTER ⚡           "
    echo "              Divisi Flasher / Hardware Serial Engineer               "
    echo "======================================================================"
    echo -e "${NC}"
}

function check_prerequisites() {
    # Check Python pyserial
    if ! python3 -c "import serial" 2>/dev/null; then
        echo -e "${YELLOW}[!] Python 'pyserial' belum terpasang. Menginstall via pip...${NC}"
        python3 -m pip install pyserial 2>/dev/null || sudo pacman -S --noconfirm python-pyserial 2>/dev/null || sudo apt-get install -y python3-serial 2>/dev/null
    fi
}

function menu() {
    local FW_VER="1.0.05"
    local VER_FILE="${BASE_DIR}/../config/WAP610N_VERSION"
    if [ -f "${VER_FILE}" ]; then
        FW_VER=$(grep "^FIRMWARE_VERSION=" "${VER_FILE}" | cut -d'"' -f2)
        [ -z "${FW_VER}" ] && FW_VER="1.0.05"
    fi

    while true; do
        show_banner

        # Deteksi status USB UART (/dev/ttyUSB* atau /dev/ttyACM*)
        USB_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n 1)
        if [ -n "${USB_PORT}" ]; then
            echo -e "🔌 Status USB UART : ${GREEN}${BOLD}TERDETEKSI (${USB_PORT})${NC}"
        else
            USB_PORT="/dev/ttyUSB0"
            echo -e "🔌 Status USB UART : ${RED}${BOLD}TIDAK TERDETEKSI (Colok USB TTL CP2102/CH340)${NC}"
        fi

        # Deteksi status firmware binary
        FW_BOOTP="${OUTPUT_DIR}/bootpImage"
        FW_BIN="${OUTPUT_DIR}/WAP610N_v${FW_VER}.bin"
        if [ -f "${FW_BOOTP}" ]; then
            FW_FILE="${FW_BOOTP}"
        elif [ -f "${FW_BIN}" ]; then
            FW_FILE="${FW_BIN}"
        elif [ -f "${BASE_DIR}/../output/WAP610N_v${FW_VER}.bin" ]; then
            FW_FILE="${BASE_DIR}/../output/WAP610N_v${FW_VER}.bin"
        else
            FW_FILE="${FW_BOOTP}"
        fi

        if [ -f "${FW_FILE}" ]; then
            FW_SIZE=$(stat -c "%s" "${FW_FILE}" 2>/dev/null || wc -c < "${FW_FILE}")
            FW_MB=$(awk -v s="${FW_SIZE}" 'BEGIN {printf "%.2f", s/1048576}')
            echo -e "📦 Firmware Binary  : ${GREEN}Siap Flash [$(basename "${FW_FILE}")] (${FW_SIZE} bytes / ${FW_MB} MB)${NC}"
        else
            echo -e "📦 Firmware Binary  : ${RED}File biner belum ada di firmware/ atau ../output/${NC}"
        fi
        echo "----------------------------------------------------------------------"
        echo -e "${BOLD}PILIHAN OPERASI FIRMWARE FLASHER:${NC}"
        echo -e "  ${CYAN}[1]${NC} 🚀 ${BOLD}Flashing Otomatis 1-Klik${NC} (Dual-Mode: TFTP LAN + Fallback UART Kermit)"
        echo -e "  ${CYAN}[2]${NC} 🔍 ${BOLD}Audit & Validasi Biner Firmware${NC} (Header 32-byte, Device ID, CRC)"
        echo -e "  ${CYAN}[3]${NC} 💻 ${BOLD}Buka Serial Terminal Interaktif${NC} (U-Boot CLI / Linux Shell)"
        echo -e "  ${CYAN}[4]${NC} 📋 ${BOLD}Live Serial Logger & Capture${NC} (Simpan boot log real-time ke file)"
        echo -e "  ${CYAN}[5]${NC} 🌐 ${BOLD}Jalankan Standalone TFTP Server${NC} (Port 69 UDP)"
        echo -e "  ${CYAN}[6]${NC} 🔧 ${BOLD}Fix Izin Port Serial & Set IP LAN${NC} (chmod 666 & IP 192.168.1.100)"
        echo -e "  ${CYAN}[7]${NC} 📖 ${BOLD}Buka Dokumentasi SOP Flasher${NC} (README.md)"
        echo -e "  ${CYAN}[0]${NC} 🚪 Keluar"
        echo "----------------------------------------------------------------------"
        read -p "Pilih menu [0-7]: " opt

        case $opt in
            1)
                echo -e "\n${GREEN}[*] Menjalankan Auto-Flasher...${NC}"
                check_prerequisites
                python3 "${TOOLS_DIR}/flash_auto.py" --port "${USB_PORT}" "${FW_FILE}"
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            2)
                echo -e "\n${GREEN}[*] Memvalidasi integritas biner firmware...${NC}"
                python3 "${TOOLS_DIR}/verify_firmware.py" "${FW_FILE}"
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            3)
                echo -e "\n${GREEN}[*] Membuka terminal serial interaktif (38400 baud)...${NC}"
                check_prerequisites
                python3 "${TOOLS_DIR}/uboot_terminal.py" --port "${USB_PORT}"
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            4)
                echo -e "\n${GREEN}[*] Menjalankan serial logger real-time...${NC}"
                check_prerequisites
                python3 "${TOOLS_DIR}/serial_logger.py" --port "${USB_PORT}"
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            5)
                echo -e "\n${GREEN}[*] Menjalankan TFTP Server lokal pada port 69 (Butuh Sudo)...${NC}"
                sudo python3 "${TOOLS_DIR}/tftp_server.py" "${OUTPUT_DIR}"
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            6)
                echo -e "\n${GREEN}[*] Memperbaiki izin port serial & setting IP LAN...${NC}"
                if ls /dev/ttyUSB* /dev/ttyACM* 1>/dev/null 2>&1; then
                    sudo chmod 666 /dev/ttyUSB* /dev/ttyACM* 2>/dev/null && echo "[+] Izin port serial (/dev/ttyUSB*, /dev/ttyACM*) berhasil disetel ke 666."
                else
                    echo -e "${YELLOW}[!] Tidak ada perangkat /dev/ttyUSB* atau /dev/ttyACM* yang terpasang.${NC}"
                fi
                echo "Interface Ethernet fisik yang terdeteksi:"
                ip -br link | awk '!/^(lo|wl|tailscale|podman|docker|veth|br-|virbr)/ {print "  - " $1 " (" $2 ")"}'
                read -p "Masukkan nama interface untuk set static 192.168.1.100/24 (kosongkan untuk batal): " ETH_IF
                if [ -n "${ETH_IF}" ]; then
                    read -p "Konfirmasi: flush IP lama pada '${ETH_IF}' dan set 192.168.1.100/24? [y/N]: " CONFIRM_IF
                    if [ "${CONFIRM_IF}" = "y" ] || [ "${CONFIRM_IF}" = "Y" ]; then
                        sudo ip addr flush dev "${ETH_IF}" 2>/dev/null
                        sudo ip addr add 192.168.1.100/24 dev "${ETH_IF}" 2>/dev/null
                        sudo ip link set "${ETH_IF}" up 2>/dev/null
                        echo -e "[+] Interface ${ETH_IF} disetel ke IP 192.168.1.100!"
                    else
                        echo "[*] Perubahan IP pada ${ETH_IF} dibatalkan."
                    fi
                fi
                read -p "Tekan [Enter] untuk kembali ke menu..."
                ;;
            7)
                less -R "${BASE_DIR}/README.md"
                ;;
            0)
                echo -e "\n${GREEN}Sampai jumpa! Firmware flasher siap beroperasi.${NC}\n"
                exit 0
                ;;
            *)
                echo -e "${RED}[!] Pilihan tidak valid.${NC}"
                sleep 1
                ;;
        esac
    done
}

check_prerequisites
menu
