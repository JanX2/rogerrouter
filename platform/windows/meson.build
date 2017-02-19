# Setup nsis installation file
cdata = configuration_data()
cdata.set('PACKAGE_NAME', '"Roger Router"')
cdata.set('PACKAGE_VERSION', '"@0@"'.format(version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')

configure_file(input : 'roger.nsi.in',
  output : 'roger.nsi',
  configuration : cdata)
