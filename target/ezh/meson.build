gen = [
  decodetree.process('insn.decode', extra_args: [ '--decode', 'decode_insn',
                                                  '--insnwidth', '32' ])
]

ezh_ss = ss.source_set()
ezh_system_ss = ss.source_set()

ezh_ss.add(gen)
ezh_ss.add(files(
  'translate.c',
  'helper.c',
  'cpu.c',
  'disas.c'))

ezh_common_system_ss = ss.source_set()
ezh_common_system_ss.add(files('gdbstub.c'))

ezh_system_ss.add(files('machine.c'))

target_arch += {'ezh': ezh_ss}
target_system_arch += {'ezh': ezh_system_ss}
target_common_system_arch += {'ezh': ezh_common_system_ss}
