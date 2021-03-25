# Socket Programming

## Client Side - TCP Socket
* The code includes the client side programme to get the HTML content of a URL through a Squid Proxy Server.
* URL and Proxy details and filenames to save content are taken as arguments.
* Code also manages redirects and saves output in HTML and for particular websites even an image logo.
* Clear and modular code is included and can be understood by function definitions.

## Input Format
$ gcc http_proxy_download.c -o http_proxy_download.out

Arguments 
1) Website URL
2) IP of Squid Proxy Server
3) Port
4) Username for Proxy
5) Password for Proxy
6) Name for HTML output
7) Name for img output

$ ./http_proxy_downoad.out info.in2p3.fr 182.75.45.22 13128 csf303 csf303 index.html logo.gif
