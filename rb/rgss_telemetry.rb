module Telemetry
  VERSION = "1.6"
  IS_TEST_VERSION = true

  ON_START = Win32API.new("rgss_telemetry", "on_start", "ppi", "v")
  ON_ERROR = Win32API.new("rgss_telemetry", "on_error", "ppp", "v")

  def self.on_start
    get_private_profile_string = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
    buffer = [].pack("x256")
    l = get_private_profile_string.call("Telemetry", "Url", "", buffer, buffer.size, "./Game.ini")
    url = buffer[0, l]
    return if url.nil? or url.empty?

    ON_START.call(url, VERSION, IS_TEST_VERSION)
  end

  def self.on_error(typename, message, stack)
    stack.each { |e|
      e.gsub!(/\{(\d+)\}\:(\d+)/i) { |m|
        "Script #{$1} -- #{ScriptNames[$1.to_i]}, Line: #{$2}"
      }
    }
    ON_ERROR.call(typename, message, stack.join("\n"))
  end
end

alias base_rgss_main rgss_main

def rgss_main(*args, &block)
  begin
    base_rgss_main(*args, &block)
  rescue => e
    Telemetry.on_error(e.class.to_s, e.to_s, e.backtrace)
  end
end

Telemetry.on_start

ScriptNames = {}

$RGSS_SCRIPTS.each_with_index { |s, i| ScriptNames[i] = s[1] }
