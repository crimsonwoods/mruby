MRuby::Gem::Specification.new('mruby-pthread') do |spec|
  spec.license = 'MIT'
  spec.author  = 'mruby developers'
  spec.summary = 'multi-threading support powered by pthread library'
  spec.linker.libraries << 'pthread'
  spec.add_dependency('mruby-thread', :core => 'mruby-thread')
end
