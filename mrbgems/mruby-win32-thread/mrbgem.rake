MRuby::Gem::Specification.new('mruby-win32-thread') do |spec|
  spec.license = 'MIT'
  spec.author  = 'mruby developers'
  spec.summary = 'multi-threading support for win32/win64 environment.'
  spec.add_dependency('mruby-thread', :core => 'mruby-thread')
end
