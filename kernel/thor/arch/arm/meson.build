thor_sources += files(
	'cpu.cpp',
	'user-access.S',
	'paging.cpp',
	'system.cpp',
	'ints.cpp',
	'pci_io.cpp',
	'debug.cpp',
	'entry.S',
	'stubs.S',
	'timer.cpp',
	'gic.cpp',
	'../../system/dtb/dtb.cpp'
)

thor_includes += include_directories('/usr/local/aarch64-none-elf/include/c++/10.1.0')
thor_includes += include_directories('.')

extra_link_args += [
	'-Wl,-T,' + meson.current_source_dir() + '/link.x'
]

extra_cpp_args += [
	'-DCXXSHIM_INTEGRATE_GCC',
	'-mgeneral-regs-only',
	'-mno-red-zone'
]

extra_c_args += [
	'-mgeneral-regs-only',
	'-mno-red-zone'
]

extra_link_depends += [files('link.x')]
