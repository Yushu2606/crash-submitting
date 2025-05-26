module Telemetry
  # 发行版本
  VERSION = "1.7"

  ON_START = Win32API.new("rgss_telemetry", "on_start", "pp", "v")
  ON_ERROR = Win32API.new("rgss_telemetry", "on_error", "ppp", "v")

  def self.on_start
    get_private_profile_string = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
    buffer = [].pack("x260")
    l = get_private_profile_string.call("Telemetry", "Url", "", buffer, buffer.size, "./Game.ini")
    @@url = buffer[0, l]
    return if self.unactive?

    ON_START.call(@@url, VERSION)
  end

  def self.on_error(typename, message, stack)
    false if self.unactive?

    stack.each { |e|
      e.gsub!(/(?:(?:\{(\d+?)\})?:(\d+?)|(\w+?)):in `(.+?)'/i) { |m|
        "{\"index\":\"#{$1}\",\"script\":\"#{$3 || ScriptNames[$1.to_i]}\",\"line\":\"#{$2}\",\"function\":\"#{$4}\"}"
      }
    }
    ON_ERROR.call(typename, message, stack.join("\n"))
    true
  end

  def self.unactive?
    @@url.nil? or @@url.empty?
  end
end

alias base_rgss_main rgss_main

def rgss_main(*args, &block)
  begin
    base_rgss_main(*args, &block)
  rescue => e
    raise unless Telemetry.on_error(e.class.to_s, e.to_s, e.backtrace)
  end
end

ScriptNames = {}

$RGSS_SCRIPTS.each_with_index { |s, i| ScriptNames[i] = s[1] }

Telemetry.on_start
