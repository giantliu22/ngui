
import 'ngui/url'
import { P, M } from './test'

console.log('\nOutputs:\n');

var uri = new url.URL('http://nodegui.org/home/index.html?a=a&b=b#c=c&d=d');

// new url.URL('http://nodegui.org:/home/index.html?a=a&b=b#c=c&d=d').href;

for ( var i in uri ) {
	var r = uri[i];
	if ( typeof r != 'function' ) {
		console.log(i, ': ', r);
	}
}

console.log('\nURL::Propertys:\n');

P(uri, 'href');
P(uri, 'filename');
P(uri, 'dirname');
P(uri, 'search');
P(uri, 'hash');
P(uri, 'host');
P(uri, 'hostname');
P(uri, 'origin');
P(uri, 'basename');
P(uri, 'extname');
P(uri, 'port');
P(uri, 'protocol');
P(uri, 'params');
P(uri, 'hashParams');

console.log('\nURL::Methods:\n');

M(uri, 'get', ['a']);
M(uri, 'set', ['a', 'A'], 'href');
M(uri, 'del', ['a'], 'href');
M(uri, 'clearParam', [], 'href');
M(uri, 'getHash', ['a']);
M(uri, 'setHash', ['k', 'K'], 'href');
M(uri, 'setHash', ['k', 'K'], 'href');
M(uri, 'clearHash', [], 'href');
M(uri, 'relative', ['http://nodegui.org/A/B/C/test.js']);

console.log('\nMethods:\n');

M(url, 'executable');
M(url, 'documents');
M(url, 'temp');
M(url, 'resources');
M(url, 'restore', [url.resources()]);
M(url, 'cwd');
M(url, 'chdir', ['/']);
M(url, 'cwd');
M(url, 'search', ['http://nodegui.org/?a=100']);
M(url, 'hash', ['http://nodegui.org/?a=100&b=test#a=200&b=300']);
M(url, 'filename', ['file:///a/b/c/kk.jsx']);
M(url, 'dirname', ['file:///a/b/c/kk.jsx']);
M(url, 'host', ['a/b/c/kk.jsx']);
M(url, 'host', ['http://nodegui.org/a/b/c/kk.jsx']);
M(url, 'hostname', ['a/b/c/kk.jsx']);
M(url, 'hostname', ['http://nodegui.org/a/b/c/kk.jsx']);
M(url, 'origin', ['a/b/c/kk.jsx']);
M(url, 'origin', ['http://nodegui.org/a/b/c/kk.jsx']);
M(url, 'basename', ['a/b/c/kk.jsx']);
M(url, 'extname', ['a/b/c/kk.jsx']);
M(url, 'port', ['http://nodegui.org:81/a/b/c/kk.jsx']);
M(url, 'protocol', ['a/b/c/kk.jsx']);
M(url, 'protocol', ['http://nodegui.org/a/b/c/kk.jsx']);
M(url, 'protocol', ['lib://util/fs']);
M(url, 'protocol', [':util/fs']);
M(url, 'protocol', ['zip:///var/data/test.apk@/assets/index']);
M(url, 'params', ['http://nodegui.org/?a=100&b=test#a=200&b=300']);
M(url, 'hashParams', ['http://nodegui.org/?a=100&b=test#a=200&b=300']);
M(url, 'get', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a']);
M(url, 'set', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a', 'A']);
M(url, 'del', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a']);
M(url, 'clearParam', ['http://nodegui.org/?a=100&b=test#a=200&b=300']);
M(url, 'getHash', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a']);
M(url, 'setHash', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a', 'H']);
M(url, 'delHash', ['http://nodegui.org/?a=100&b=test#a=200&b=300', 'a']);
M(url, 'clearHash', ['http://nodegui.org/?a=100&b=test#a=200&b=300']);
M(url, 'relative', ['http://nodegui.org/home', 'http://nodegui.org/A/B/C/test.js']);
M(url, 'isAbsolute', ['http://nodegui.org/home/index.html']);
M(url, 'isAbsolute', ['file:///a/b/c/kk.jsx']);
M(url, 'isAbsolute', ['file:///d:/a/b/c/kk.jsx']);
M(url, 'isAbsolute', ['/a/b/c/kk.jsx']);
M(url, 'isAbsolute', ['d:/a/b/c/kk.jsx']);
M(url, 'isAbsolute', ['c/kk.jsx']);
M(url, 'resolve', ['http://nodegui.org/home', '..', 'A', 'B', '..', 'C', 'test.js']);
