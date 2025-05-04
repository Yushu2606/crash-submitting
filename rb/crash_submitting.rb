GetPrivateProfileInt = Win32API.new("kernel32", "GetPrivateProfileString", "ppppip", "i")
buffer = [].pack('x256')
l = GetPrivateProfileInt.call("Crash", "Url", "", buffer, buffer.size, "./Game.ini")
url = buffer[0, l]
raise if url == ""
start_hook = Win32API.new("crash_submitting", "start_hook", "p", "i")
hook_result = start_hook.call(url)
raise if hook_result != 0
