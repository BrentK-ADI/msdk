#include "board.h"
#include "board_api.h"
#include "mcr_regs.h"
#include "mxc_sys.h"

void board_init(void)
{
    MXC_SYS_ClockSourceEnable(MXC_SYS_CLOCK_IPO);
    MXC_MCR->ldoctrl |= MXC_F_MCR_LDOCTRL_0P9EN;
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_USB);
    MXC_SYS_Reset_Periph(MXC_SYS_RESET0_USB);
}

void USB_IRQHandler(void)
{
   // MXC_USB_EventHandler();
   tud_int_handler(0);
}
