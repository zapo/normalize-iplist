require File.dirname(__FILE__) + "/helper.rb"

class NormalizeIPListValidateTest < Test::Unit::TestCase
  def test_validate_empty_array_is_empty_array
    assert_equal([], NormalizeIPList.validate([]))
  end

  def test_validate_by_default_returns_only_one
    assert_equal(['192.168.0.'],
                 NormalizeIPList.validate(['192.168.0.1/32',
                                           '192.168.0.',
                                           '192.168._',
                                           'foo',
                                           '192.168.0.1/32',
                                           '192.168.0.1']))
  end

  def test_validate_valid_range
    assert_equal([],
                 NormalizeIPList.validate(['192.168.0.1,192.168.0.10']))
  end

  def test_validate_range_with_invalid_endpoint
    assert_equal(['192.168.0.1,192.168.0.256'],
                 NormalizeIPList.validate(['192.168.0.1,192.168.0.256']))
  end

  def test_validate_with_high_values
    assert_equal(['272.208.76.24'],
                 NormalizeIPList.validate(['272.208.76.24']))
  end

  def test_validate_range_with_high_values
    assert_equal(['255.255.255.255,300.0.0.0'],
                 NormalizeIPList.validate(['255.255.255.255,300.0.0.0']))
  end

  def test_validate_with_supplied_n_returns_up_to_n
    assert_equal(['192.168.0.', '192.168._', 'foo'],
                 NormalizeIPList.validate(['192.168.0.1/32',
                                           '192.168.0.',
                                           '192.168._',
                                           'foo',
                                           '192.168.0.',
                                           'f192.168.0.1'],
                                          3))
  end

  def test_validate_with_various_wellformed_ips_returns_empty_array
    assert_equal([],
                 NormalizeIPList.validate(['1.2.3.4', '10.0.0.0/8', '3.3.3.3/32', '1.2.0.0/16'], 3))
  end
end
