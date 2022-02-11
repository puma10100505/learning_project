因为.gitignore配置了exe文件不能上传，所以这里将nginx做成了压缩包，使用前先解压
将本目录下的nginx-1.20.2.zip解压到当前目录即可使用
启动nginx后在浏览器输入http://perfetto.ui:10000(配置host)或http://localhost:10000就可以打开perfetto-ui

更推荐的方式(系统中要先安装有python2/python3)：
进入Tools/perfetto/perfetto-ui目录，在命令行执行: python3 -m http.server [port:8000]以当前目录启动简易的http服务器，启动完成后可以在浏览器中通过http://localhost:8000访问
如果是python2可以使用: python -m SimpleHTTPServer [port:8000] 