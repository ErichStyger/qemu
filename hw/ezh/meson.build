ezh_ss = ss.source_set()
ezh_ss.add(files('boot.c'))
ezh_ss.add(when: 'CONFIG_EZH', if_true: files('ezh.c'))
#ezh_ss.add(files('ezh.c'))
ezh_ss.add(when: 'CONFIG_MCXN947', if_true: files('mcxn947.c'))
#ezh_ss.add(files('mcxn947.c'))

hw_arch += {'ezh': ezh_ss}
