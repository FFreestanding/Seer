#include <kernel_io/memory.h>
#include <data_structure/kernel_bitset.h>
#include <stdarg.h>


void
memory_copy_fast(uint32_t* source, uint32_t* destination, uint32_t length)
{
    while (length--)
    {
        (*destination) = (*source);
        source++;
        destination++;
	}
}

void
memory_copy(uint8_t* source, uint8_t* destination, uint32_t length)
{
    while (length--)
    {
        (*destination) = (*source);
        source++;
        destination++;
		// kernel_log(INFO, "source %h, destination %h, memory_copy_fast %h", source, destination,length);
    }
}

void
memory_set_fast(uint32_t* source, uint32_t value, uint32_t length)
{
    while (length--)
    {
        (*source) = value;
        source++;
    }
}

void
memory_set(uint8_t* source, uint8_t value, uint32_t length)
{
    while (length--)
    {
        (*source) = value;
        source++;
    }
}

// %c(character) %u(unsigned int) %i(int) %s(string) %h(hexadecimal)
void
kernel_log(const uint8_t log_type, const uint8_t* information, ...)
{
    VGA_Manager* log_vga = get_vga_manager_instance();
	switch (log_type)
	{
	case INFO:
		vga_set_theme(log_vga, VGA_COLOR_GREEN, VGA_COLOR_BLACK);
		break;
	case WARN:
		vga_set_theme(log_vga, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
		break;
	case ERROR:
		vga_set_theme(log_vga, VGA_COLOR_RED, VGA_COLOR_BLACK);
		break;
	}
	va_list args;
	va_start(args, information);

	while (*information != '\0')
	{
		if (*information != '%')
		{
            vga_putchar(log_vga, *information);
			++information;
			continue;
		}

		++information;
		switch (*information)
		{
		case 'c':
			char c1 = va_arg(args, int);
			vga_putchar(log_vga, c1);
			break;
		case 'u':
			log_uint(log_vga, va_arg(args, uint32_t));
			break;
		case 'i':
            log_int(log_vga, va_arg(args, int));
			break;
		case 's':
			char* c = va_arg(args, char*);
			while (*c != '\0')
			{
				vga_putchar(log_vga, *c);
				++c;
			}
			break;
		case 'h':
			vga_putstr(log_vga, "0x");
			log_hex(log_vga, va_arg(args, uint32_t));
			break;
		}

		++information;
	}
	vga_putchar(log_vga, '\n');
    tty_sync_cursor();
}


void log_hex(VGA_Manager* log_vga, uint32_t n){
	if (n / 16) {log_hex(log_vga, n / 16);}
	if (n % 16 > 9)
	{
		vga_putchar(log_vga, n % 16 + 48 + 7);
	}
	else
	{
    	vga_putchar(log_vga, n % 16 + 48);
	}
}

void log_int(VGA_Manager* log_vga, int n){
	if (n < 0) {
		n = -n;
        vga_putchar(log_vga, '-');
	}
	if (n / 10) {log_int(log_vga, n / 10);}
    vga_putchar(log_vga, n % 10 + 48);
}

void log_uint(VGA_Manager* log_vga, uint32_t n){
	if (n / 10) {log_uint(log_vga, n / 10);}
    vga_putchar(log_vga, n % 10 + 48);
}

void
kprintf(const uint8_t* information, ...)
{
	VGA_Manager* log_vga = get_vga_manager_instance();
	vga_set_theme(log_vga, VGA_COLOR_BROWN, VGA_COLOR_BLACK);
	va_list args;
	va_start(args, information);
	while (*information != '\0')
	{
		if (*information != '%')
		{
            vga_putchar(log_vga, *information);
			++information;
			continue;
		}

		++information;
		switch (*information)
		{
		case 'c':
			char c1 = va_arg(args, int);
			vga_putchar(log_vga, c1);
			break;
		case 'u':
			log_uint(log_vga, va_arg(args, uint32_t));
			break;
		case 'i':
            log_int(log_vga, va_arg(args, int));
			break;
		case 's':
			char* c = va_arg(args, char*);
			while (*c != '\0')
			{
				vga_putchar(log_vga, *c);
				++c;
			}
			break;
		case 'h':
			vga_putstr(log_vga, "0x");
			log_hex(log_vga, va_arg(args, uint32_t));
			break;
		}

		++information;
	}
}

