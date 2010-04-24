(function($) { 
$.speak = { 
	'connections': {}, 'from_id': {}, 'introductions': {}, 'requests': [], 'timeout': null,
	'debug': function(){},
	'defaults': { 'server': 'http://speak.lusion.co.za', 'json': false	},
	'request': function(action,data,callback) {
		var req = {'aborted': false};
		req.abort = function() { req.aborted = true; };
		if (!data['from_id']) data['from_id'] = 0;
		$.speak.debug("request:",action,data);
		$.getJSON($.speak.defaults.server+"/"+action+"?"+$.param(data)+"&time="+ (new Date()).getTime()+"&callback=?",function(d) {
			$.speak.debug("callback:",action,data,"json:",d,"aborted:",req.aborted);
			if (!req.aborted) {
				if (d.error_code == 'AUTH') {
					$.speak.debug("lost connection:",data['channel']);
					// Reintroduce (lost connection), then try the request again (force reauth)
					$.speak.introduce(channel,$.speak.introductions[data['channel']],function(auth){ 
						$.speak.debug("reintroduced, retrying request");
						$.speak.request(action,$.extend(data,auth),callback);
					}, true);
				}else if (d.result == 'timeout') $.speak.request(action,data,callback);
				else if (callback) callback(d);	
			}
		});
		$.merge($.speak.requests,[req]);
		return req;
	}
};
$.speak.json = {};
$.speak.abort = function() {
	$.each($.speak.requests,function() { this.abort(); });
};
$.speak.reset = function() {
	$.speak.abort();
	$.merge($.speak,{'connections':{},'from_id':{},'introductions':{}});
};
$.speak.hold = function(channel) {
	$.speak.load(channel,function() {
		$.speak.hold(channel);
	});
};
$.speak.send = function(channel,message,callback) {
	var data = $.extend({},$.speak.connections[channel]);
	data['channel'] = channel;
	data['message'] = message;
	$.speak.request('write',data,callback);
};
$.speak.json.send = function(channel,message,callback) {return $.speak.send(channel,$.toJSON(message),callback);};

$.speak.json.load = function(channel,callback) {
	return $.speak.load(channel, function (json) {
			for (k in json.messages) {
				if (json.messages[k].message) json.messages[k].message = $.secureEvalJSON(json.messages[k].message);
			}
			callback(json);
		});
};

$.speak.load = function (channel,callback) {
	var data = $.extend({},$.speak.connections[channel]);
	data['channel'] = channel;
	data['from_id'] = $.speak.from_id[channel];
	if ($.speak.timeout) data['timeout'] = $.speak.timeout;
	$.speak.request('read',data,function (json) {
		$.speak.from_id[channel] = json['last_id']+1;
		callback(json);
	});
};

$.speak.introduce = function (channel, message, callback, force) {
	if (force == undefined) force = false;
	var data = {'channel':channel, 'message':message};
	if (!force) $.extend(data,$.speak.connections[channel]); // if its not a reintroduction
	$.speak.introductions[channel] = message;
	$.speak.request('introduce',data,function(json) {
		if (json.auth) {
			$.speak.connections[channel] = {'auth':json.auth,'connection':json.connection};
			$.speak.debug("auth received",channel,$.speak.connections[channel]);
			if (callback) callback($.speak.connections[channel]);
		}else if (callback) callback(false);
	});
};
$.speak.json.introduce = function(channel,message,callback) {return $.speak.introduce(channel,$.toJSON(message),callback);};

$.speak.create = function (callback) {
	$.speak.request('create',{},callback);
}

$.speak.connect = function (channel, introduction) {
	var c = {'channel': channel, 'users': {}, 'message':function(){}, 'hello':function(){}, 'goodbye':function(){}};

	c.load = function() {
		$.speak.json.load(c.channel, function(json) {
			$.each(json.messages,function () {
				if (this.type == 'introduction') {
					var n = c.users[this.from] ? false : true; // is new?
					c.users[this.from] = this.message;
					c.hello(this.from, c.users[this.from], n);
				}else if (this.type == 'goodbye') {
					if (this.from && c.users[this.from]) {
						c.goodbye(this.from, c.users[this.from]);
						c.users[this.from] = null;
					}
				}else{
					c.message(this.from, this.from ? c.users[this.from] : null, this.message);
				}
			});
			c.load();
		});
	};
	c.write = function(message, callback) {
		$.speak.json.send(c.channel, message, callback);
	}
	c.introduce = function(introduction) {
		$.speak.json.introduce(c.channel, introduction);	
	};

	$.speak.json.introduce(channel, introduction, function(conn) {
		c.me = conn['connection'];
		c.load(); // start loading	
	});
	return c;
};


})(jQuery);
