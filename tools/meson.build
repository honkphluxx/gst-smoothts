exe_args = common_args + []

# sources used to compile this program
tool_sources = [
  'gstsmoothts.c',
]

executable('gstsmoothts',
  tool_sources,
  install: true,
  c_args : exe_args,
  include_directories: [configinc],
  dependencies : tool_deps,
)
