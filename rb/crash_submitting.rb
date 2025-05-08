def main
  return unless File.exist?("./crash_submitting.dll")

  getPrivateProfileString = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
  buffer = [].pack('x256')
  l = getPrivateProfileString.call("Crash", "Url", "", buffer, buffer.size, "./Game.ini")
  url = buffer[0, l]
  return if url.nil? or url.empty?

  start_hook = Win32API.new("crash_submitting", "start_hook", "p", "i")
  hook_result = start_hook.call(url)
  raise unless hook_result == 0
  return start_hook
end
START_HOOK = main