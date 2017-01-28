cdata = configuration_data()
cdata.set('VERSION', rm_version)
configure_file(input : 'version.xml.in',
  output : 'version.xml',
  configuration : cdata)

gnome.gtkdoc('rm',
  main_xml: 'rm-docs.xml',
  src_dir: '@0@/rm'.format(meson.source_root()),
  scan_args: [
    '--rebuild-types',
    '--ignore-headers=config.h plugins'
  ],
  install: true)
