# Helper macro for generating otp variants
# OPFILE : path to the otp file
# REG_FILE: script that generates the otp
# OUT_FILE : Generated file path
macro(add_assemble_pcie_boot_config)
  cmake_parse_arguments(PCIE
    ""
    "OPFILE;REG_FILE;OUT_FILE"
    ""
    ${ARGN}
    )

  get_filename_component(OPFILE_ABS_PATH ${PCIE_OPFILE} ABSOLUTE)
  get_filename_component(REGFILE_ABS_PATH ${PCIE_REG_FILE} ABSOLUTE)

  # Create target that depends on the generated files
  add_custom_target(assemble-pcie-boot-config-${PCIE_OUT_FILE} ALL
    DEPENDS
        ${PCIE_OUT_FILE}
    )

  add_custom_command(
    OUTPUT ${PCIE_OUT_FILE}
    COMMAND
        ${DEVICE_ARTIFACTS_TOOLS_DIR}/config_rom_assembler.py
            --outfile ${PCIE_OUT_FILE}
            --opfile ${OPFILE_ABS_PATH}
            --regfile ${REGFILE_ABS_PATH}
    DEPENDS
        ${DEVICE_ARTIFACTS_TOOLS_DIR}/config_rom_assembler.py
        ${PCIE_OPFILE}
        ${PCI_REG_FILE}
   )
endmacro(add_assemble_pcie_boot_config)