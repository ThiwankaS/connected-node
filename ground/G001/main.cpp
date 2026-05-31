#include <cstdint>
#include <stddef.h>

// direct memory mapped register address from ESP32 reference manual
// --- MEMORY-MAPPED REGISTERS FOR THE GPIO PERIPHERAL ---
constexpr uint32_t GPIO_ENABLE_W1TS_REG     = 0x3FF44024; // write 1 to enable output
constexpr uint32_t GPIO_OUT_W1TS_REG        = 0x3FF44008; // write 1 to force PIN to HIGH
constexpr uint32_t GPIO_OUT_W1TC_REG        = 0x3FF4400C; // write 1 to force PIN to LOW
constexpr uint32_t IO_MUX_GPIO2_REG         = 0x3FF49040; // IO MUX configuration register
// --- MEMORY-MAPPED REGISTERS FOR SAFETY WATCHDOGS ---
constexpr uint32_t RTC_CNTL_WDTWPROTECT_REG = 0x3FF480A4; // write-protection lock register for the Real-Time Clock (RTC)
constexpr uint32_t RTC_CNTL_WDTCONFIG0_REG  = 0x3FF4808C; // primary configuration/control register for the Real-Time Clock (RTC)
constexpr uint32_t TIMG0_WDTWPROTECT_REG    = 0x3FF5F064; // write-protection lock register for Timer Group 0's (TIMG0)
constexpr uint32_t TIMG0_WDTCONFIG0_REG     = 0x3FF5F048; // primary configuration/control register for Timer Group 0's (TIMG0)
constexpr uint32_t TIMG1_WDTWPROTECT_REG    = 0x3FF60064; // write-protection lock register for Timer Group 1's (TIMG1)
constexpr uint32_t TIMG1_WDTCONFIG0_REG     = 0x3FF60048; // primary configuration/control register for Timer Group 1's (TIMG1)
constexpr uint32_t WDT_WKEY_VALUE           = 0x50D83AA1; // hardware "magic key" code password
// --- PERIPHERAL APPLICATION CONFIGURATIONS ---
constexpr uint32_t LED_PIN                  = 2;          // board's built-in LED on GPIO2
constexpr uint32_t IO_MUX_GPIO2_FUNC        = 2u << 12;   // MCU_SEL = GPIO function

// zero-cost hardware layout template pattern
template <uint32_t Address>
struct RawRegister {
    // A inline static method designed to write a value straight into a hardware address with zero function-call overhead
    static inline void write(uint32_t value) {
        *reinterpret_cast<volatile uint32_t*>(Address) = value;
    }
    // A inline static method designed to read a value straight from a hardware address with zero function-call overhead
    static inline uint32_t read() {
        return *reinterpret_cast<volatile uint32_t*>(Address);
    }
};

static inline void disable_watchdogs() {

    // --- PERIPHERAL APPLICATION CONFIGURATIONS ---
    // ROM leaves flash-boot watchdogs enabled; disable before the blink loop runs.

    // --- DISABLE RTC WATCHDOG TIMER ---
    // Writes the unique 32-bit magic safety key password to unlock modification access to the RTC Watchdog register
    RawRegister<RTC_CNTL_WDTWPROTECT_REG>::write(WDT_WKEY_VALUE);
    // Reads current RTC configuration, clears Bit 31 (Global Enable) and Bit 10 (Flash Boot Protection), then writes it back
    RawRegister<RTC_CNTL_WDTCONFIG0_REG>::write(
        RawRegister<RTC_CNTL_WDTCONFIG0_REG>::read() & ~((1u << 31) | (1u << 10)));
    // Writes zero to the RTC protection register to instantly lock it back up from accidental software writes
    RawRegister<RTC_CNTL_WDTWPROTECT_REG>::write(0);

    // --- DISABLE TIMER GROUP 0 (TIMG0) WATCHDOG TIMER ---
    // Writes the unique 32-bit magic safety key password to unlock modification access to the TIMG0 Watchdog register
    RawRegister<TIMG0_WDTWPROTECT_REG>::write(WDT_WKEY_VALUE);
    // Reads TIMG0 configuration, clears Bit 31 (Global Enable) and Bit 14 (Flash Boot Protection), then writes it back
    RawRegister<TIMG0_WDTCONFIG0_REG>::write(
        RawRegister<TIMG0_WDTCONFIG0_REG>::read() & ~((1u << 31) | (1u << 14)));
    // Writes zero to the TIMG0 protection register to instantly lock it back up from accidental software writes
    RawRegister<TIMG0_WDTWPROTECT_REG>::write(0);

    // --- DISABLE TIMER GROUP 1 (TIMG1) WATCHDOG TIMER ---
    // Writes the unique 32-bit magic safety key password to unlock modification access to the TIMG1 Watchdog register
    RawRegister<TIMG1_WDTWPROTECT_REG>::write(WDT_WKEY_VALUE);
    // Reads TIMG1 configuration, clears Bit 31 (Global Enable) and Bit 14 (Flash Boot Protection), then writes it back
    RawRegister<TIMG1_WDTCONFIG0_REG>::write(
        RawRegister<TIMG1_WDTCONFIG0_REG>::read() & ~((1u << 31) | (1u << 14)));
    // Writes zero to the TIMG0 protection register to instantly lock it back up from accidental software writes
    RawRegister<TIMG1_WDTWPROTECT_REG>::write(0);
}

// Entry point called by boot.S; extern "C" turns off C++ name-mangling so the raw assembly linker can find it
extern "C" void bare_metal_main() {
   
    // Calls watchdog disabling function to kill the safety timers so the chip doesn't auto-reset during infinite loops
    disable_watchdogs();

    // Writes function selection data to the IO MUX register to physically link Pin 2 to the GPIO matrix subsystem
    RawRegister<IO_MUX_GPIO2_REG>::write(IO_MUX_GPIO2_FUNC);

    // Shifts a 1 left by 2 positions to set Bit 2 in the Enable register, transforming GPIO Pin 2 into an Output
    RawRegister<GPIO_ENABLE_W1TS_REG>::write(1U << LED_PIN);

    // Starts the infinite, non-terminating bare-metal loop that will control execution forever
    while (true) {
        // Sets Bit 2 inside the atomic Write-1-to-Set Output register to drive physical Pin 2 HIGH (Turns LED ON)
        RawRegister<GPIO_OUT_W1TS_REG>::write(1U << LED_PIN);
        // A standard delay loop using a volatile index variable so the compiler is strictly forbidden from optimizing it away
        for(volatile int i = 0; i < 500000; ++i) {
            // Interjects a raw assembly No Operation instruction to explicitly waste 1 internal CPU clock cycle per loop pass
            __asm__ volatile("nop"); 
        }

        // Sets Bit 2 inside the atomic Write-1-to-Clear Output register to drive physical Pin 2 LOW (Turns LED OFF)
        RawRegister<GPIO_OUT_W1TC_REG>::write(1U << LED_PIN);
        // A standard delay loop using a volatile index variable so the compiler is strictly forbidden from optimizing it away
        for(volatile int i = 0; i < 500000; ++i) { 
            // Interjects a raw assembly No Operation instruction to explicitly waste 1 internal CPU clock cycle per loop pass
            __asm__ volatile("nop"); 
        }
    }
}
