#include "acpi.h"
#include "common.h"
#include "port.h"
#include "debug.h"

#define sleep(x) for(int _=0;_<x;++_){asm("nop");}

namespace acpi {

static u32 *SMI_CMD;
static u8 ACPI_ENABLE;
//static u8 ACPI_DISABLE;
static u32 *PM1a_CNT;
static u32 *PM1b_CNT;
static u16 SLP_TYPa;
static u16 SLP_TYPb;
static u16 SLP_EN;
static u16 SCI_EN;
//static u8 PM1_CNT_LEN;



struct PACKED RSDP {
	u8 signature[8];
	u8 check_sum;
	u8 oem_id[6];
	u8 revision;
	u32 rsdt;
};

struct PACKED RSDP2 {
	RSDP base;
	u32 length;
	u64 xsdt;
	u8 extended_checksum;
	u8 reserved[3];
};

bool valid(RSDP *rsdp) {
	u16 sum = 0;
	for (u8 *byte = (u8 *)rsdp; byte < (u8 *)(rsdp + 1); ++byte) {
		sum += *byte;
	}
	if ((sum & 0xff) == 0) {
		if (rsdp->revision == 0) {
			return true;
		} else if (rsdp->revision == 2) {
			auto rsdp2 = (RSDP2 *)rsdp;
			sum = 0;
			for (u8 *byte = (u8 *)rsdp2; byte < (u8 *)(rsdp2 + 1); ++byte) {
				sum += *byte;
			}
			if ((sum & 0xff) == 0) {
					return true;
			}
		}
	}
	return false;
}

RSDP *get_rsdp() {
	auto address_to_check = (u8 *)0x000e0000;
	while (address_to_check < (u8 *)0x00100000) {
		if (memory_equals(address_to_check, "RSD PTR ", 8)) {
			auto rsdp = (RSDP *)address_to_check;
			if (valid(rsdp)) {
				return rsdp;
			}
		}
		address_to_check += 16;
	}
	return 0;
}


struct FACP {
	u8 Signature[4];
	u32 Length;
	u8 unneded1[40 - 8];
	u32 *DSDT;
	u8 unneded2[48 - 44];
	u32 *SMI_CMD;
	u8 ACPI_ENABLE;
	u8 ACPI_DISABLE;
	u8 unneded3[64 - 54];
	u32 *PM1a_CNT_BLK;
	u32 *PM1b_CNT_BLK;
	u8 unneded4[89 - 72];
	u8 PM1_CNT_LEN;
};


// check if the given address has a valid header
unsigned int *checkRSDP(unsigned int *ptr) {
	char const *sig = "RSD PTR ";
	RSDP *rsdp = (RSDP *)ptr;
	u8 *bptr;
	u8 check = 0;

	if (memory_equals(sig, rsdp, 8))    {
	   // check checksum rsdpd
		bptr = (u8 *)ptr;
		for (u32 i = 0; i < (u32)sizeof(RSDP); i++) 	  {
			check += *bptr;
			bptr++;
		}

		// found valid rsdpd
		if (check == 0) {
		   /*
			if (desc->Revision == 0)
			  debug_print("acpi 1"s);
		   else
			  debug_print("acpi 2"s);
		   */
			return (unsigned int *)rsdp->rsdt;
		}
	}

	return 0;
}

// finds the acpi header and returns the address of the rsdt
unsigned int *getRSDP(void) {
	unsigned int *addr;
	unsigned int *rsdp;

	// search below the 1mb mark for RSDP signature
	for (addr = (unsigned int *)0x000E0000; (int)addr < 0x00100000; addr += 0x10 / sizeof(addr))    {
		rsdp = checkRSDP(addr);
		if (rsdp != 0)
			return rsdp;
	}


	// at address 0x40:0x0E is the RM segment of the ebda
	int ebda = *((short *)0x40E);   // get pointer
	ebda = ebda * 0x10 & 0x000FFFFF;   // transform segment into linear address

	// search Extended BIOS Data Area for the Root System Description Pointer signature
	for (addr = (unsigned int *)ebda; (int)addr < ebda + 1024; addr += 0x10 / sizeof(addr))    {
		rsdp = checkRSDP(addr);
		if (rsdp != 0)
			return rsdp;
	}

	return 0;
}



// checks for a given header and validates checksum
int checkHeader(unsigned int *ptr, char const *sig) {
	if (memory_equals(ptr, sig, 4))    {
		char *checkPtr = (char *)ptr;
		int len = *(ptr + 1);
		char check = 0;
		while (0 < len--) 	  {
			check += *checkPtr;
			checkPtr++;
		}
		if (check == 0)
			return 0;
	}
	return -1;
}



int enable(void) {
   // check if acpi is enabled
	if ((port::read_u16((unsigned int)PM1a_CNT) & SCI_EN) == 0)    {
	   // check if acpi can be enabled
		if (SMI_CMD != 0 && ACPI_ENABLE != 0) 	  {
			port::write_u8((unsigned int)SMI_CMD, ACPI_ENABLE); // send acpi enable command
			// give 3 seconds time to enable acpi
			int i;
			for (i = 0; i < 300; i++) 		 {
				if ((port::read_u16((unsigned int)PM1a_CNT) & SCI_EN) == 1)
					break;
				sleep(10);
			}
			if (PM1b_CNT != 0)
				for (; i < 300; i++) 			{
					if ((port::read_u16((unsigned int)PM1b_CNT) & SCI_EN) == 1)
						break;
					sleep(10);
				}
			if (i < 300) {
				debug_print("enabled acpi\n"s);
				return 0;
			} else {
				debug_print("couldn't enable acpi\n"s);
				return -1;
			}
		} else {
			debug_print("no known way to enable acpi\n"s);
			return -1;
		}
	} else {
	   //debug_print("acpi was already enabled\n"s);
		return 0;
	}
}



#if 1
bool init() {
	auto rsdp = get_rsdp();
	if (rsdp) {
		debug_print("Found RSDP revision "s);
		debug_print(rsdp->revision);
		debug_print(".\n"s);
		if (rsdp->revision == 0) {

		} else {
		}
		return true;
	}
	debug_print("RSDP not found\n"s);
	return false;
}
#else
//
// bytecode of the \_S5 object
// -----------------------------------------
//        | (optional) |    |    |    |
// NameOP | \          | _  | S  | 5  | _
// 08     | 5A         | 5F | 53 | 35 | 5F
//
// -----------------------------------------------------------------------------------------------------------
//           |           |              | ( SLP_TYPa   ) | ( SLP_TYPb   ) | ( Reserved   ) | (Reserved    )
// PackageOP | PkgLength | NumElements  | byteprefix Num | byteprefix Num | byteprefix Num | byteprefix Num
// 12        | 0A        | 04           | 0A         05  | 0A          05 | 0A         05  | 0A         05
//
//----this-structure-was-also-seen----------------------
// PackageOP | PkgLength | NumElements |
// 12        | 06        | 04          | 00 00 00 00
//
// (Pkglength bit 6-7 encode additional PkgLength bytes [shouldn't be the case here])
//
bool init() {
	unsigned int *ptr = getRSDP();

	// check if address is correct  ( if acpi is available on this pc )
	if (ptr != 0 && checkHeader(ptr, "RSDT") == 0)    {
	   // the RSDT contains an unknown number of pointers to acpi tables
		int entrys = *(ptr + 1);
		entrys = (entrys - 36) / 4;
		ptr += 36 / 4;   // skip header information

		while (0 < entrys--) 	  {
		   // check if the desired table is reached
			if (checkHeader((unsigned int *)*ptr, "FACP") == 0) 		 {
				entrys = -2;
				FACP *facp = (FACP *)*ptr;
				if (checkHeader((unsigned int *)facp->DSDT, "DSDT") == 0) 			{
				   // search the \_S5 package in the DSDT
					char *S5Addr = (char *)facp->DSDT + 36; // skip header
					int dsdtLength = *(facp->DSDT + 1) - 36;
					while (0 < dsdtLength--) 			   {
						if (memory_equals(S5Addr, "_S5_", 4))
							break;
						S5Addr++;
					}
					// check if \_S5 was found
					if (dsdtLength > 0) 			   {
					   // check for valid AML structure
						if ((*(S5Addr - 1) == 0x08 || (*(S5Addr - 2) == 0x08 && *(S5Addr - 1) == '\\')) && *(S5Addr + 4) == 0x12) 				  {
							S5Addr += 5;
							S5Addr += ((*S5Addr & 0xC0) >> 6) + 2;   // calculate PkgLength size

							if (*S5Addr == 0x0A)
								S5Addr++;   // skip byteprefix
							SLP_TYPa = *(S5Addr) << 10;
							S5Addr++;

							if (*S5Addr == 0x0A)
								S5Addr++;   // skip byteprefix
							SLP_TYPb = *(S5Addr) << 10;

							SMI_CMD = facp->SMI_CMD;

							ACPI_ENABLE = facp->ACPI_ENABLE;
							ACPI_DISABLE = facp->ACPI_DISABLE;

							PM1a_CNT = facp->PM1a_CNT_BLK;
							PM1b_CNT = facp->PM1b_CNT_BLK;

							PM1_CNT_LEN = facp->PM1_CNT_LEN;

							SLP_EN = 1 << 13;
							SCI_EN = 1;

							return true;
						} else {
							debug_print("\\_S5 parse error\n"s);
						}
					} else {
						debug_print("\\_S5 not present\n"s);
					}
				} else {
					debug_print("DSDT invalid\n"s);
				}
			}
			ptr++;
		}
		debug_print("no valid FACP present\n"s);
	} else {
		debug_print("no acpi\n"s);
	}

	return false;
}
#endif



void power_off() {
   // SCI_EN is set to 1 if acpi shutdown is possible
	if (SCI_EN == 0)
		return;

	enable();

	// send the shutdown command
	port::write_u16((unsigned int)PM1a_CNT, SLP_TYPa | SLP_EN);
	if (PM1b_CNT != 0)
		port::write_u16((unsigned int)PM1b_CNT, SLP_TYPb | SLP_EN);

	debug_print("acpi poweroff failed\n"s);
}

void restart() {
	//port::write_u8(FADT->ResetReg.Address, FADT->ResetValue);
}

}
