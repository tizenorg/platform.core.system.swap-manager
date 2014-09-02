if (window.tizen == undefined) {
	alert("Tizen API is not supported by this browser");
	throw ("Tizen API is not supported by this browser");
}

(function(global) {

	var AUTOMATIC_PROBES = 2;
	var PROBE_LIST = [
	{
	    objects: [
	    {
	        name: "window",
	        depth: 2
	    }, ],
	    functions: [],
	    delay: 0
	},
	{
	    objects: [
	    {
	        name: "window",
	        depth: 2
	    }, ],
	    functions: [],
	    delay: 1000
	}, ];

	var PROBE_BLACKLIST = [ "Object", // throws exception
	"Date", // crashes
	"log_fd", "trace_fd", "probe" ];

	// try to set more probes
	var UNSAFE = true;

	var in_probe = false;

// ----------------------------------------------------------------

	var log_fd, trace_fd;
	var start_time = performance.webkitNow();

	function log(msg) {
		try {
			s = arguments.callee.caller.name;
		} catch (e) {
			s = "null";
		}

		log_fd.write("[D] (" + s + "): " + msg + "\n");
	}

	function err(e) {
		try {
			s = arguments.callee.caller.name;
		} catch (er) {
			s = "null";
		}

		log_fd.write("[E] (" + s + "): " + e.message + "\n");
	}

	function trace(str) {
		trace_fd.write(str);
	}

	var objProps = objectProperties;
	function objectProperties(obj) {
		var lst = Object.getOwnPropertyNames(obj);
		var arr = [];

		for ( var name in obj)
			arr.push(obj[name]);

		for ( var i = 0; i < lst.length; ++i)
			arr.push(lst[i]);

		return arr;
	}

	function timeDiffToStr(t1, t2) {
		var dt = t2 - t1;
		var t_min = Math.floor(dt / 60000);
		dt -= (60000 * t_min);
		var t_sec = Math.floor(dt / 1000);
		dt -= (1000 * t_sec);
		var t_usec = Math.floor(dt * 1000);
		if (t_min < 10)
			t_min = "0" + t_min;
		if (t_sec < 10)
			t_sec = "0" + t_sec;
		if (t_usec < 1000)
			t_usec = "000" + t_usec;
		if (t_usec < 10000)
			t_usec = "0" + t_usec;
		if (t_usec < 100000)
			t_usec = "0" + t_usec;
		return t_min + ":" + t_sec + "." + t_usec;
	}

	function timeDiff(t1, t2) {
		var dt = t2 - t1;
		var sec = Math.floor(dt / 1000);
		dt -= (1000 * sec);
		var nsec = Math.floor(dt * 1000000);
		return (
		{
		    sec: sec,
		    nsec: nsec
		});
	}

	function initLog(_fd) {
		if (log_fd != undefined) {
			_fd.write(log_fd.writeBuf);
			_fd.writeBytes(log_fd.writeBytesBuf);
		}
		log_fd = _fd;
	}

	function initTrace(_fd) {
		if (trace_fd != undefined) {
			_fd.write(trace_fd.writeBuf);
			_fd.writeBytes(trace_fd.writeBytesBuf);
		}
		trace_fd = _fd;
	}

	function temp_fd() {
		this.writeBuf = "";
		this.writeBytesBuf = [];

		this.write = function(s) {
			var u8 = new Uint8Array(1);

			for ( var i = 0; i < s.length; i++) {
				u8[0] = s.charCodeAt(i);
				this.writeBytesBuf.push(u8[0]);
			}
//			this.writeBuf += s;
		}

		this.writeBytes = function(a) {
			for ( var i = 0; i < a.length; i++) {
				this.writeBytesBuf.push(a[i]);
			}
//			this.writeBytesBuf.push(a);
		}
	}

	function errAlert(e) {
		alert(e.message);
	}

	function createFiles(dir) {
		log_fn = "js_log";
		trace_fn = "js_trace";
		try {
			log_file = dir.createFile(log_fn);
		} catch (e) {
			log_file = dir.resolve(log_fn);
		}
		try {
			trace_file = dir.createFile(trace_fn);
		} catch (e) {
			trace_file = dir.resolve(trace_fn);
		}
		log_file.openStream("w", initLog, errAlert);
		trace_file.openStream("w", initTrace, errAlert);
	}

	function isValidFunction(o, f) {
		if (PROBE_BLACKLIST.indexOf(f) > -1)
			return false;
		if (typeof o[f] !== "function")
			return false;
		if (!UNSAFE) {
			if (!Object.getOwnPropertyDescriptor(o, f).writable)
				return false;
			if (("" + o[f]).indexOf("[native code]") >= 0)
				return false;
		}
		return true;
	}

	function isValidObject(o, p) {
		if (PROBE_BLACKLIST.indexOf(p) > -1)
			return false;
		if (typeof o[p] !== "object")
			return false;
		if (o[p] === null)
			return false;
		if (!UNSAFE) {
			if (!Object.getOwnPropertyDescriptor(o, p).writable)
				return false;
		}
		try {
			if ("" + o[p] === "undefined")
				return false;
		} catch (e) {
			return false;
		}
		return true;
	}

// ----------------------------------------------------------------

	function uint16(buffer, byteOffset, littleEndian) {
		var dv = new DataView(buffer, byteOffset);
		this.byteOffset = byteOffset;
		littleEndian = true;

		this.set = function(value) {
			dv.setUint16(0, value, littleEndian);
		}

		this.get = function() {
			return dv.getUint16(0, littleEndian);
		}

		this.add = function(value) {
			return dv.setUint16(0, dv.getUint16(0, littleEndian) + value, littleEndian);
		}
	}

	function uint32(buffer, byteOffset, littleEndian) {
		var dv = new DataView(buffer, byteOffset);
		this.byteOffset = byteOffset;
		littleEndian = true;

		this.set = function(value) {
			dv.setUint32(0, value, littleEndian);
		}

		this.get = function() {
			return dv.getUint32(0, littleEndian);
		}

		this.add = function(value) {
			return dv.setUint32(0, dv.getUint32(0, littleEndian) + value, littleEndian);
		}
	}

	var packet = {};
	var seq = 0;

	packet.arrayBuffer = new ArrayBuffer(1024);
	packet.arrayBytes = new Uint8Array(packet.arrayBuffer);
	packet.dataView = new DataView(packet.arrayBuffer, 0);

	packet.marker = new uint16(packet.arrayBuffer, 0);
	packet.length = new uint16(packet.arrayBuffer, 2);

	packet.id = new uint32(packet.arrayBuffer, 4);
	packet.seq = new uint32(packet.arrayBuffer, 8);
	packet.sec = new uint32(packet.arrayBuffer, 12);
	packet.nsec = new uint32(packet.arrayBuffer, 16);
	packet.len = new uint32(packet.arrayBuffer, 20);
	packet.func = new Uint8Array(packet.arrayBuffer, 24);

	packet.marker.set(0xFACE);

	function probe(func, msg_id, args) {
		if (in_probe)
			return;
		in_probe = true;

		td = timeDiff(start_time, performance.webkitNow());

		var len = packet.func.byteOffset;

		packet.id.set(msg_id);
		packet.seq.set(++seq);
		packet.sec.set(td.sec);
		packet.nsec.set(td.nsec);

		for (i = 0; i < func.length; i++)
			packet.func[i] = func.charCodeAt(i);
		packet.func[i++] = 0;
		len += i;

		packet.dataView.setUint32(len, args.length, true);
		len += 4;

		Array.prototype.forEach.call(args, function(e, i, a) {
			switch (typeof (e)) {
			case "number":
				packet.dataView.setUint8(len++, "d".charCodeAt(0));
				packet.dataView.setUint32(len, e, true);
				len += 4;
				return;
			case "boolean":
				packet.dataView.setUint8(len++, "b".charCodeAt(0));
				packet.dataView.setUint8(len++, e ? 1 : 0);
				return;
			case "string":
			case "object":
				s = "" + e;
				break;
			case "function":
			default:
				s = typeof (e);
			}
			packet.dataView.setUint8(len++, "s".charCodeAt(0));
			if (s.length >= packet.arrayBytes.length - len)
				s = "Very very very long string (" + s.length  + " bytes)";
			for (i = 0; i < s.length; i++)
				packet.dataView.setUint8(len++, s.charCodeAt(i));
			packet.dataView.setUint8(len++, 0);				
		});

		packet.length.set(len);
		packet.len.set(len - packet.func.byteOffset);

		for (var arr = [], i = 0; i < len; i++)
			arr.push(packet.arrayBytes[i]);
		trace_fd.writeBytes(arr);

		in_probe = false;
	}

	function setProbe(obj, func, prefix) {
		if (typeof obj[func].__hook__ !== "undefined") {
//			log("function already hooked: " + obj + "." + func);
			return;
		}
		log(prefix + func);
		var tmp = obj[func];
		try {
			obj[func] = function() {
				var hook_id = arguments.callee.__hook_id__;
				probe(hook_id, 0x0017, arguments);
				var ret = arguments.callee.__hook__.apply(this, arguments);
				probe(hook_id, 0x0018, [ ret ]);
				return ret;
			}
			obj[func].__hook__ = tmp;
			obj[func].__hook_id__ = (prefix + func);
			var lst = Object.getOwnPropertyNames(tmp);
			for ( var i = 0; i < lst.length; ++i) {
				obj[func][lst[i]] = tmp[lst[i]];
			}
		} catch (e) {
			err(e);
			obj[func] = tmp;
		}
	}

	function setProbesBreadthFirst(obj, prefix, depth) {
		var queue = [
		{
		    o: obj,
		    p: prefix,
		    d: depth
		} ], qs = 0, qe = 1;
		while (qe > qs) {
			var item = queue[qs];
			++qs;
			function process(o) {
				var old_qe = qe;
				try {
					if (isValidFunction(item.o, o)) {
						setProbe(item.o, o, item.p);
					} else if (isValidObject(item.o, o)) {
						if (item.d > 1) {
							queue.push(
							{
							    o: item.o[o],
							    p: item.p + o + ".",
							    d: item.d - 1
							});
							++qe;
						}
					}
				} catch (e) {
					err(e);
					qe = old_qe;
				}
			}
			var items = {};
			var lst = Object.getOwnPropertyNames(item.o);
			for ( var i = 0; i < lst.length; ++i)
				items[lst[i]] = null;
			if (UNSAFE) {
				for ( var o in item.o)
					items[o] = null;
			}
			for (i in items)
				process(i);
		}
	}

// ----------------------------------------------------------------

	global.setProbePack = function(x) {
		function do_setProbes(x) {
//			log("Set probes (" + x + ")");
			in_probe = true;

			for ( var i = 0; i < PROBE_LIST[x].functions.length; ++i) {
				try {
					var o = PROBE_LIST[x].functions[i];
//					log("function[" + i + "] " + o.name + "." + o.func);
					setProbe(eval(o.name), o.func, o.name + ".");
				} catch (e) {
					err(e);
				}
			}

			for ( var i = 0; i < PROBE_LIST[x].objects.length; ++i) {
				try {
					var o = PROBE_LIST[x].objects[i];
					log("object[" + i + "]");
					setProbesBreadthFirst(eval(o.name), o.name + ".", o.depth);
				} catch (e) {
					err(e);
				}
			}

			in_probe = false;
		}

		if (PROBE_LIST[x].delay) {
			setTimeout(function() {
				do_setProbes(x);
			}, PROBE_LIST[x].delay);
		} else {
			do_setProbes(x);
		}
	}

// ----------------------------------------------------------------

	initLog(new temp_fd());
	initTrace(new temp_fd());

	try {
		tizen.filesystem.resolve("wgt-private", createFiles, errAlert);
	} catch (e) {
		errAlert(e);
	}

	global.window.onerror = function() {
		log("[E] arguments: " + Array.prototype.join.call(arguments, "; "));
	}

	log("Time diff: " + timeDiffToStr(start_time, performance.webkitNow()) + "; Before setting probes");

	for ( var i = 0; i < AUTOMATIC_PROBES; ++i) {
		setProbePack(i);
	}

	log("Time diff: " + timeDiffToStr(start_time, performance.webkitNow()) + "; After setting probes");

})(this);
