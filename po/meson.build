langs = ['de']

podir = '@0@'.format(meson.current_source_dir())

i18n = import('i18n')
i18n.gettext(meson.project_name(), languages : langs, args: ['-k_', '--from-code=UTF-8'])

