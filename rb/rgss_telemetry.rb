VERSION = "1.5"
START_TELEMETRY = Win32API.new("rgss_telemetry", "start_hook", "pp", "i")

def main
  getPrivateProfileString = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
  buffer = [].pack("x256")
  l = getPrivateProfileString.call("Telemetry", "Url", "", buffer, buffer.size, "./Game.ini")
  url = buffer[0, l]
  return if url.nil? or url.empty?

  start_result = START_TELEMETRY.call(url, VERSION)
  raise "Telemetry startup failed." unless start_result == 0
  return url
end

URL = main
