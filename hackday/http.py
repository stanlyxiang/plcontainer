#!/usr/bin/env python
"""
Very simple HTTP server in python.
Usage::
    ./dummy-web-server.py [<port>]
Send a GET request::
    curl http://localhost
Send a HEAD request::
    curl -I http://localhost
Send a POST request::
    curl -d "foo=bar&bin=baz" http://localhost
"""
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from urlparse import urlparse
import subprocess
import urllib
import random

class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def gen_random_string(self):
	x = "garden"
	for i in range(1, 20):
	   x = x + random.choice('abcdefghijklmnopqrsuvwxzy1234567890')
	return x

    def do_GET(self):
        self._set_headers()
	query = urlparse(self.path).query
	query = urllib.unquote(query)
	query_components = dict(qc.split("=") for qc in query.split("T"))
	request = query_components["query"]
        print request
	result = "fail"
	if request == "start":
	    command_str = "/home/gpadmin/workspace/garden/gaol_linux create -m /home/gpadmin/workspace/plclientlog:/clientdir"
	    command_args = " -n " + self.gen_random_string() + " -r /var/lib/docker/aufs/diff/cc20116d23c2f4165b93ebdffe0d20dc68ba33cb40d21433207d460a1bf235f8"
	    result = subprocess.check_output(command_str + command_args, shell=True)
	    self.wfile.write(result)
	elif request == "stop":
	    command_str = "/home/gpadmin/workspace/garden/gaol_linux destroy "
	    command_args = query_components["name"]
	    result = subprocess.check_output(command_str + command_args, shell=True)
	    self.wfile.write(query_components["name"])
	elif request == "run":
	    command_str = "/home/gpadmin/workspace/garden/gaol_linux net-in -p 8080 "
	    command_args = query_components["name"]
	    result = subprocess.check_output(command_str + command_args, shell=True)
	    map_port = result.split(':')
	    command_str = "/home/gpadmin/workspace/garden/gaol_linux run -c \"nohup /clientdir/client.sh & \" "
	    command_args = query_components["name"]
	    subprocess.check_output(command_str + command_args, shell=True)
	    self.wfile.write(map_port[1])

    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        # Doesn't do anything with posted data
        self._set_headers()
	content_length = int(self.headers['Content-Length']) # <--- Gets the size of data
        post_data = self.rfile.read(content_length) # <--- Gets the data itself
	self.wfile.write("<html><body><h1>POST!</h1><pre>" + post_data + "</pre></body></html>")       
 
def run(server_class=HTTPServer, handler_class=S, port=8088):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print 'Starting httpd...'
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()
