
#include <cstdio>
#include <sapi/hal.hpp>
#include <sapi/sys.hpp>
#include <sapi/var.hpp>

typedef struct {
	u8 slave_addr;
	int page_size;
	u16 size;
	const char * filename;
} options_t;

void show_usage(const Cli & cli);
void read_eeprom(const I2C & i2c, const options_t & options);
void write_eeprom(const I2C & i2c, const options_t & options);
void erase_eeprom(const I2C & i2c, const options_t & options);

int main(int argc, char * argv[]){
	Cli cli(argc, argv);
	cli.set_publisher("Stratify Labs, Inc");
	cli.handle_version();

	int port = 0;
	I2CPinAssignment pin_assignment;
	String filename;
	bool is_ch = false;
	options_t options;
	int bitrate = 100000;

	options.page_size = 8;
	options.size = 4096;
	options.slave_addr = 0x50;
	options.filename = 0;

	if( cli.is_option("-i2c") ){
		port = cli.get_option_value("-i2c");
	}

	if( cli.is_option("-s") ){
		options.size = cli.get_option_value("-s");
	}

	if( cli.is_option("-a") ){
		options.slave_addr = cli.get_option_value("-a");
	}

	if( cli.is_option("-scl") ){
		pin_assignment->scl = cli.get_option_pin("-scl");
	}

	if( cli.is_option("-sda") ){
		pin_assignment->scl = cli.get_option_pin("-sda");
	}

	if( cli.is_option("-b") ){
		bitrate = cli.get_option_value("-b");
	}

	if( cli.is_option("-ps") ){
		options.page_size = cli.get_option_value("-ps");
		if( options.page_size > 256 ){
			options.page_size = 256;
		}
	}

	I2C i2c(port);

	if( i2c.init(I2C::FLAG_SET_MASTER, bitrate, pin_assignment) < 0 ){
		printf("Error: Failed to initialize I2C bus %d\n", port);
		exit(0);
	}

	if( cli.is_option("-r") ){

		if( cli.is_option("-o") ){
			filename = cli.get_option_argument("-o");
		} else if( cli.is_option("-ch") ){
			is_ch = true;
		} else {
			printf("Error: must specify -o or -ch with -r option\n");
			show_usage(cli);
		}

		printf("Read I2C EEPROM to %s on I2C %d at %d bps\n",
				is_ch ? "STDOUT" : filename.c_str(),
						port,
						bitrate
		);



		if( !is_ch ){
			options.filename = filename.c_str();
		}

		read_eeprom(i2c, options);

	} else if( cli.is_option("-w") ){
		if( cli.is_option("-i") ){
			filename = cli.get_option_argument("-i");
		} else {
			printf("Error: must specify -i filename with -w\n");
			show_usage(cli);
		}

		options.filename = filename.c_str();

		write_eeprom(i2c, options);

	} else if( cli.is_option("-e") ){
		erase_eeprom(i2c, options);
	} else {
		show_usage(cli);
	}

	return 0;
}

void read_eeprom(const I2C & i2c, const options_t & options){
	File f;
	bool is_header = false;
	int ret;
	int bytes;
	char buffer[options.page_size];
	int i;

	if( options.filename == 0 ){
		//write output as a C header to stdout
		is_header = true;
	} else {
		if( f.create(options.filename) < 0 ){
			printf("Failed to create %s\n", options.filename);
			return;
		}
	}

	i2c.prepare(0x50, I2C::FLAG_PREPARE_PTR_DATA | I2C::FLAG_IS_PTR_16);
	bytes = 0;
	if( is_header ){
		printf("const char data[] = {\n");
	}
	do {

		ret = i2c.read(bytes, buffer, options.page_size);
		if( ret > 0 ){
			if( is_header ){
				printf("\t");
				for(i=0; i < options.page_size; i++){
					printf("0x%02X,", buffer[i]);
				}
				printf("//0x%04X\n", bytes);
			} else {
				printf(".");
				fflush(stdout);
				if( f.write(bytes, buffer, ret) != ret ){
					printf("Failed to write file at %d bytes\n", bytes);
					return;
				}
			}
			bytes += ret;
		}

	} while( (ret == options.page_size) && (bytes < options.size) );

	if( is_header ){
		printf("};\n\n");
	} else {
		f.close();
		printf("Done\n");
	}


}

void write_eeprom(const I2C & i2c, const options_t & options){
	File f;
	int bytes;
	char buffer[options.page_size];
	int ret;

	if( f.open(options.filename, File::RDONLY) < 0 ){
		printf("Failed to open file %s\n", options.filename);
		return;
	}


	printf("Writing EEPROM from %s", options.filename);
	fflush(stdout);
	bytes = 0;
	do {

		if( f.read(bytes, buffer, options.page_size) != options.page_size ){
			printf("Failed to read image at 0x%04X\n", bytes);
			return;
		}

		printf(".");
		fflush(stdout);

		ret = i2c.write(bytes, buffer, options.page_size);

		if( ret > 0 ){
			bytes += ret;
		}

	} while( ret == options.page_size && bytes < options.size );

	printf("Done\n");

}

void erase_eeprom(const I2C & i2c, const options_t & options){
	int bytes;
	char buffer[options.page_size];
	int ret;


	printf("Erasing EEPROM");
	fflush(stdout);
	bytes = 0;
	memset(buffer, 0xff, options.page_size);
	do {

		printf(".");
		fflush(stdout);

		ret = i2c.write(bytes, buffer, options.page_size);

		if( ret > 0 ){
			bytes += ret;
		}

	} while( ret == options.page_size && bytes < options.size );

	printf("Done\n");
}


void show_usage(const Cli & cli){
	printf("%s usage:\n", cli.name());
	printf("%s [-r|-w] [-i filename] [-o filename] [-i2c port] [-pu] [-b bitrate] [-sda X.Y] [-scl X.Y] [-ch] [-ps page_size]\n", cli.name());
	printf("\t-r read the EEPROM to -o filename\n");
	printf("\t -ch can be used with -r to dump the contents as a C header file to the STDOUT\n");
	printf("\t-w write the EEPROM using binary file specified by -i filename\n");
	printf("\t-i2c specify which I2C port to use\n");
	printf("\t-sda X.Y use port X and pin Y for SDA line\n");
	printf("\t-scl X.Y use port X and pin Y for SCL line\n");
	printf("\t-pu use internal pullup resistors on I2C bus if available\n");
	printf("\t-b specify the I2C bitrate to set when reading/writing\n");
	printf("\t-ps specify the page size for reading/writing (default is 32 bytes)\n");
	exit(0);
}
