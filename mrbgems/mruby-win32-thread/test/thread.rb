assert('Thread.new') do
  t = Thread.new do
    'test'
  end
  assert_false t.nil?
  assert_equal 'test', t.join
end

assert('Thread.join') do
  t = Thread.new do
    # return nil when no expression is evaluated.
  end
  assert_nil t.join
end

assert('multiple threads') do
  t1 = Thread.new do
    i = 0
    100.times do
      i = i + 1
    end
    i
  end
  t2 = Thread.new do
    i = 0
    200.times do
      i = i + 1
    end
  end
  assert_equal 100, t1.join
  assert_equal 200, t2.join
end

assert('parameterized thread') do
  n = 1
  t1 = Thread.new(n) do |x|
    10.times do
      x = x + 1
    end
    x
  end
  n = 11
  t2 = Thread.new(n) do |x|
    10.times do
      x = x + 1
    end
    x
  end
  assert_equal 11, t1.join
  assert_equal 21, t2.join
end

assert('GC working while thread is running') do
  t = Thread.new() do
    x = 0
    10000.times do
      x = x + 1
    end
    x
  end
  assert_equal 10000, t.join
end

assert('access to global variables') do
  $gvar = 10
  t = Thread.new() do
    $gvar = $gvar + 1
  end
  assert_equal 11, t.join
  assert_equal 11, $gvar
end

assert('access to global variables from multiple threads') do
  $gvar = 0
  t1 = Thread.new() do
    100.times do
      $gvar = $gvar + 1
    end
  end
  t2 = Thread.new() do
    100.times do
      $gvar = $gvar + 1
    end
  end
  t1.join
  t2.join
  assert_true $gvar > 0
  assert_true $gvar <= 200
end

assert('access to symbol table') do
  class Test
    def test
      1
    end
  end
  t = Thread.new(Test.new, :test) do |obj, sym|
    obj.send sym
  end
  assert_equal 1, t.join
end

assert('exception is thrown inside of thread') do
  t = Thread.new do
    raise 'error'
  end
  begin
    t.join
    assert_true false # must not be reached here.
  rescue RuntimeError => e
    assert_equal 'error', e.message
  end
end

assert('access to captured variable') do
  var = 0
  t = Thread.new do
    100.times do
      var = var + 1
    end
  end
  t.join
  assert_equal var, 100
end

assert('create thread inside of thread') do
  t1 = Thread.new do
    Thread.new do
      1
    end
  end
  t2 = t1.join
  assert_false t2.nil?
  assert_true t2.kind_of?(Thread)
  assert_equal 1, t2.join
end

assert('create thread and join') do
  t = Thread.new do
    t = Thread.new do
      1
    end
    t.join
  end
  assert_equal 1, t.join
end
