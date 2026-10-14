# .gdbinit

# Define your application arguments exactly as they would appear on the command line
set args --r="ge 1:1"

# (Optional) You can also set environment variables if your C code checks them
# used fot serve_tskjv_gemtext.cgi
set env QUERY_STRING=/?ge%201%3A1

# used fot serve_tskjv_html.fcgi
#set env QUERY_STRING=?references=ge%201%3A1

# (Optional) Automatically place a breakpoint at main on startup
# break main

echo \n>>> Local .gdbinit loaded. Arguments configured successfully! <<<\n\n
