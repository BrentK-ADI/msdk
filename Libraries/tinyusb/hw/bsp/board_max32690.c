#include "board.h"
#include "board_api.h"


void board_init(void)
{
    
}
void USB_IRQHandler(void)
{
   MXC_USB_EventHandler();
   tud_int_handler(0);
}
