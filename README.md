# gba-sram-bank-pinner
Small patch for saving on 1M SRAM bootlegs

GBA bootlegs with 1M SRAM split SRAM into two banks of 512K. The bank is unitialized at boot so either may be selected. This can result in spooky behavior like a game appaearing to switch between two different save files at random. 
This patch adds a small stub of code to make sure the bank is always set to 0 on boot. 