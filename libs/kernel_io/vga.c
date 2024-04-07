#include <kernel_io/vga.h>
#include <kernel_io/memory.h>
#include <apic/cpu.h>

void 
vga_manager_init(VGA_Manager* vga_manager)
{
    vga_set_theme(vga_manager, VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_manager->vga_ptr = (unsigned short*) VGA_VIRTUAL_ADDRESS;
    vga_clear(vga_manager);
}

VGA_Manager*
get_vga_manager_instance()
{
    static VGA_Manager vga_manager;
    return &vga_manager;
}

void
vga_putchar(VGA_Manager* vga_manager, char vga_character)
{
    if (vga_character==0){ return; }
    switch (vga_character)
    {
    case '\t':
        vga_manager->vga_x += 4;
        break;
    case '\n':
        vga_manager->vga_y++;
        vga_manager->vga_x = 0;
        break;
    case '\r':
        vga_manager->vga_x = 0;
        break;
    }
    if (vga_manager->vga_x >= VGA_WIDTH)
    {
        vga_manager->vga_x = 0;
        vga_manager->vga_y++;
    }
    if (vga_manager->vga_y >= VGA_HEIGTH)
    {
        vga_scroll_up();
        vga_manager->vga_y--;
    }
    if (vga_character!='\t' && vga_character!='\n' && vga_character!='\r')
    {
        *(vga_manager->vga_ptr + vga_manager->vga_x + vga_manager->vga_y * VGA_WIDTH) = (vga_manager->vga_theme_color | vga_character);
        vga_manager->vga_x++;
    }

}

void
vga_putstr(VGA_Manager* vga_manager, char* vga_string)
{
    while (*vga_string != '\0')
    {
        vga_putchar(vga_manager, *vga_string);
        ++vga_string;
    }
}

void
vga_scroll_up()
{
    memory_copy_fast((uint32_t*)((uint16_t*)VGA_VIRTUAL_ADDRESS + VGA_WIDTH), 
                            (uint32_t*)VGA_VIRTUAL_ADDRESS, 
                            (VGA_WIDTH * (VGA_HEIGTH-1))/2);
    memory_set_fast((uint32_t*)((uint16_t*)VGA_VIRTUAL_ADDRESS + VGA_WIDTH * (VGA_HEIGTH-1)), 0, VGA_WIDTH/2);
}

void
vga_clear(VGA_Manager* vga_manager)
{
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGTH; i++)
    {
        *(vga_manager->vga_ptr+i) = vga_manager->vga_theme_color | ' ';
    }
    vga_manager->vga_x = 0;
    vga_manager->vga_y = 0;
}

void
vga_set_theme(VGA_Manager* vga_manager, VGA_COLOR vga_foreground_color, VGA_COLOR vga_background_color)
{
    vga_manager->vga_theme_color = (vga_background_color << 4 | vga_foreground_color) << 8;
}


void tty_sync_cursor() {
    VGA_Manager* v = get_vga_manager_instance();
    tty_set_cursor(v->vga_x, v->vga_y);
}


void tty_set_cursor(uint8_t x, uint8_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGTH) {
        x = y = 0;
    }
    uint32_t pos = y * VGA_WIDTH + x;
    io_outb(0x3D4, 14);
    io_outb(0x3D5, pos / 256);
    io_outb(0x3D4, 15);
    io_outb(0x3D5, pos % 256);
}


void
vga_putchar_x_y_temporarily(VGA_Manager* v, char c, uint8_t x, uint8_t y)
{
    *(v->vga_ptr + x + y * VGA_WIDTH) = (v->vga_theme_color | c);
}

void
vga_putstr_x_y_temporarily(VGA_Manager* v, char* s, uint8_t x, uint8_t y)
{
    while (*s != '\0')
    {
        vga_putchar_x_y_temporarily(v, *s, x, y);
        s++;
    }
}

void 
vga_backspace()
{
	VGA_Manager* vm = get_vga_manager_instance();
	if (vm->vga_x || vm->vga_y)
	{
		if (vm->vga_x==0)
		{
			vm->vga_x = VGA_WIDTH-1;
			--vm->vga_y;
		}
		else
		{
			--vm->vga_x;
		}
        // *(vm->vga_ptr + vm->vga_x + vm->vga_y * VGA_WIDTH) = (vm->vga_theme_color | ' ');
        
		vga_putchar(vm, ' ');
        --vm->vga_x;
	}
    else
    {
        return;
    }
    
    // kernel_log(WARN, "%h", vm->vga_x);
    uint16_t* p = (uint16_t*)VGA_VIRTUAL_ADDRESS + vm->vga_x + VGA_WIDTH*vm->vga_y + 1;
    while ((*p)!=vm->vga_theme_color && 
        p <= (uint16_t*)VGA_VIRTUAL_ADDRESS + VGA_HEIGTH*VGA_WIDTH - 1)
    {
        *(p-1)=*p;
        ++p;
    }
    
}

void 
vga_delete()
{
	VGA_Manager* vm = get_vga_manager_instance();

    uint16_t* p = (uint16_t*)VGA_VIRTUAL_ADDRESS + vm->vga_x + VGA_WIDTH*vm->vga_y + 1;
    while ((*p)!=vm->vga_theme_color && 
        p <= (uint16_t*)VGA_VIRTUAL_ADDRESS + VGA_HEIGTH*VGA_WIDTH - 1)
    {
        *(p-1)=*p;
        ++p;
    }
}


void 
vga_move_cursor_up()
{
    VGA_Manager* vm = get_vga_manager_instance();
    if (vm->vga_y!=0)
    {
        --vm->vga_y;
    }
}

void 
vga_move_cursor_left()
{
    VGA_Manager* vm = get_vga_manager_instance();
    if (vm->vga_x!=0)
    {
        --vm->vga_x;
    }
    else if (vm->vga_x==0 && vm->vga_y!=0)
    {
        vm->vga_x = (VGA_WIDTH-1);
        --vm->vga_y;
    }
}

void 
vga_move_cursor_right()
{
    VGA_Manager* vm = get_vga_manager_instance();
    if (vm->vga_x==(VGA_WIDTH-1) && vm->vga_y==(VGA_HEIGTH-1))
    {
        return;
    }
    else if (vm->vga_x!=(VGA_WIDTH-1))
    {
        ++vm->vga_x;
    }
    else if (vm->vga_x==(VGA_WIDTH-1) && vm->vga_y!=(VGA_HEIGTH-1))
    {
        vm->vga_x=0;
        ++vm->vga_y;
    }
    
}

void 
vga_move_cursor_down()
{
    VGA_Manager* vm = get_vga_manager_instance();
    if (vm->vga_y!=(VGA_HEIGTH-1))
    {
        ++vm->vga_y;
    }
}