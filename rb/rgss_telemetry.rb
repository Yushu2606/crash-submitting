VERSION = "1.4"

def main
  return unless File.exist?("./rgss_telemetry.dll")

  getPrivateProfileString = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
  buffer = [].pack("x256")
  l = getPrivateProfileString.call("Telemetry", "Url", "", buffer, buffer.size, "./Game.ini")
  url = buffer[0, l]
  return if url.nil? or url.empty?

  start_hook = Win32API.new("rgss_telemetry", "start_hook", "pp", "i")
  hook_result = start_hook.call(url, VERSION)
  raise unless hook_result == 0
  return start_hook
end

START_HOOK = main
