# eeprog

Usage:
    eeprog [-r|-w] [-i filename] [-o filename] [-i2c port] [-pu] [-b bitrate] [-sda X.Y] [-scl X.Y] [-ch] [-ps page_size]
    	-r read the EEPROM to -o filename
    	 -ch can be used with -r to dump the contents as a C header file to the STDOUT
    	-w write the EEPROM using binary file specified by -i filename
    	-i2c specify which I2C port to use
    	-sda X.Y use port X and pin Y for SDA line
    	-scl X.Y use port X and pin Y for SCL line
    	-pu use internal pullup resistors on I2C bus if available
    	-b specify the I2C bitrate to set when reading/writing
    	-ps specify the page size for reading/writing (default is 32 bytes)