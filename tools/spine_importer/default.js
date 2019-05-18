fs      = require('fs');
path    = require('path');
mkdirp  = require('../common/node_modules/mkdirp');
PNG     = require('../common/node_modules/pngjs').PNG;
util    = require('util');

function logError(err) { if (err) console.log(err); }

function sanitizeKey(key) {
    return key.replace(/\s/g, "_");
}

function getSpriteName(skinName, slotName) {
    return sanitizeKey(skinName == "default" ? slotName : util.format("%s_%s", skinName, slotName));
}

function hide_form() {
    $("#holder").show();
    $("#config").hide();
}

function generate_pgi(data, outputName, outputPath) {
	var spineObject = JSON.parse(data);
	
	mkdirp.sync(path.dirname(outputPath));

	var f = fs.createWriteStream(outputPath);

	//-------------------------------------------------------------------------
	// write the unit nodes
	//-------------------------------------------------------------------------

	f.write(util.format("{\n\"units\": {"));
	f.write(util.format("\n\t\"%s\": {", "default"));

	f.write("\n\t\t\"nodes\": {");
	for (var i in spineObject.bones) {
		var bone = spineObject.bones[i];
		var last = (i == spineObject.bones.length - 1);

		f.write(util.format("\n\t\t\t\"%s\": { ", sanitizeKey(bone.name)));

		if (bone.parent)
			f.write(util.format("\"parent\": \"%s\"", sanitizeKey(bone.parent)));

		if (bone.x || bone.y)
			f.write(util.format(", \"translation\": [%d, %d]", bone.x ? bone.x : 0, bone.y ? bone.y : 0));

		if (bone.rotation)
			f.write(util.format(", \"rotation\": %d", bone.rotation));

		if (bone.scaleX || bone.scaleY)
			f.write(util.format(", \"scale\": [%d, %d]", bone.scaleX ? bone.scaleX : 1, bone.scaleY ? bone.scaleY : 1));

		f.write("}");
		if (!last) f.write(",");
	}
	f.write("\n\t\t}\n\t}\n},\n");


	var slotNameToIndex = {};
	var skinIndex = 0;
	for (var skinName in spineObject.skins) {
		for (var slotIndex in spineObject.slots) {
			var slot = spineObject.slots[slotIndex];
			slotNameToIndex[slot.name] = slotIndex;
		}
	}

	//-------------------------------------------------------------------------
	// write sprites
	//-------------------------------------------------------------------------

	f.write("\"sprites\": {");
	var skinIndex = 0;
	for (var skinName in spineObject.skins) {
		var slotIndex = 0;
		for (var slotName in spineObject.skins[skinName]) {
			f.write(util.format("\n\t\"%s\": {", getSpriteName(skinName, slotName)));
			f.write("\n\t\t\"frames\": {");

			var attachmentIndex = 0;
			for (var attachmentName in spineObject.skins[skinName][slotName]) {
				var attachment = spineObject.skins[skinName][slotName][attachmentName];
				f.write(util.format("\n\t\t\t\"%s\": {", attachmentName));

				f.write(util.format(" \"width\": %d, \"height\" :%d", attachment.width, attachment.height));

				if (attachment.x || attachment.y)
					f.write(util.format(", \"translation\": [%d, %d]", attachment.x ? attachment.x : 0.0, attachment.y ? attachment.y : 0.0));

				if (attachment.rotation)
					f.write(util.format(", \"rotation\": %d", attachment.rotation));

				if (attachment.scaleX || attachment.scaleY)
					f.write(util.format(", \"scale\": [%d, %d]", attachment.scaleX ? attachment.scaleX : 1.0, attachment.scaleY ? attachment.scaleY : 1.0));

				f.write(util.format(", \"texture\": \"%s\"", attachment.name ? attachment.name : attachmentName));

				f.write(" }");
				if (attachmentIndex++ < Object.keys(spineObject.skins[skinName][slotName]).length - 1) f.write(",");
			}
			f.write("\n\t\t},");
			f.write(util.format("\n\t\t\"setup_frame\": \"%s\"", spineObject.slots[slotNameToIndex[slotName]].attachment));
			f.write("\n\t}");
			if (slotIndex < Object.keys(spineObject.skins[skinName]).length - 1
			 || skinIndex < Object.keys(spineObject.skins).length - 1) f.write(",");

			slotIndex++;
		}
		skinIndex++;
	}
	f.write("\n},\n");


	//-------------------------------------------------------------------------
	// write animations
	//-------------------------------------------------------------------------

	var bone_tracks = [];
	var bone_hash = {};
	var sprite_tracks = [];
	var sprite_hash = {};
	var event_tracks = [];
	var event_hash = {};

	for (var anim in spineObject.animations) {
		for (var timeline in spineObject.animations[anim].bones) {
			if (!bone_hash[timeline]) {
				bone_tracks.push(util.format("\n\t\t\t\t\"%s\": {}", sanitizeKey(timeline)));
				bone_hash[timeline] = 1;
			}
		}

		for (var timeline in spineObject.animations[anim].slots) {
			if (!sprite_hash[timeline])
				sprite_hash[timeline] = {};
			for (var i in spineObject.animations[anim].slots[timeline].attachment) {
				var keyframe = spineObject.animations[anim].slots[timeline].attachment[i];
				if (!sprite_hash[timeline][keyframe.name])
					sprite_hash[timeline][keyframe.name] = 1;
			}
		}

		for (var spriteName in sprite_hash) {
			var spriteFrames = [];
			for (var spriteFrame in sprite_hash[spriteName])
				spriteFrames.push(util.format("\"%s\"", spriteFrame));

			sprite_tracks.push(util.format("\n\t\t\t\t\"%s\": [%s]", sanitizeKey(spriteName), util.format(spriteFrames.join(", "))));
		}

		for (var timeline in spineObject.animations[anim].events) {
			if (!bone_hash[timeline]) {
				event_tracks.push(util.format("\n\t\t\t\t\"%s\": {}", sanitizeKey(timeline)));
				event_hash[timeline] = 1;
			}
		}
	}

	f.write("\"animsets\": {");
	f.write(util.format("\n\t\"%s\": {", "default"));

	f.write("\n\t\t\"tracks\": {");

	f.write("\n\t\t\t\"bone\": {");
	f.write(bone_tracks.join(","));
	f.write("\n\t\t\t},");

	f.write("\n\t\t\t\"sprite\": {");
	f.write(sprite_tracks.join(","));
	f.write("\n\t\t\t},");

	f.write("\n\t\t\t\"event\": {");
	f.write(event_tracks.join(","));
	f.write("\n\t\t\t}");

	f.write("\n\t\t},");
	f.write("\n\t\t\"animations\": {");

	var animIndex = 0;
	var numAnims = Object.keys(spineObject.animations).length;
	for (var anim in spineObject.animations) {
		f.write(util.format("\n\t\t\t\"%s\": {", anim));
		if (spineObject.animations[anim].bones) {
			f.write("\n\t\t\t\t\"bone\": {");

			var boneIndex = 0;
			var numBones = Object.keys(spineObject.animations[anim].bones).length;
			for (var bone in spineObject.animations[anim].bones) {
				f.write(util.format("\n\t\t\t\t\t\"%s\": {", sanitizeKey(bone)));

				var first = true;
				if (spineObject.animations[anim].bones[bone].translate) {
					f.write("\n\t\t\t\t\t\t\"translation\": [");
					for (var i in spineObject.animations[anim].bones[bone].translate) {
						var keyframe = spineObject.animations[anim].bones[bone].translate[i];
						f.write("\n\t\t\t\t\t\t\t{");
						f.write(util.format("\"time\": %d", keyframe.time));
						f.write(util.format(", \"value\": [%d, %d]", keyframe.x ? keyframe.x : 0.0, keyframe.y ? keyframe.y : 0.0));
						if (keyframe.curve) {
							if (typeof keyframe.curve == "string")
								f.write(util.format(", \"curve\": \"%s\"", keyframe.curve.toUpperCase()));
							else
								f.write(util.format(", \"curve\": [%s, %s, %s, %s]", keyframe.curve[0], keyframe.curve[1], keyframe.curve[2], keyframe.curve[3]));
						}
						f.write("}");
						if (i < spineObject.animations[anim].bones[bone].translate.length - 1) f.write(",");
					}
					f.write("\n\t\t\t\t\t\t]");
					first = false;
				}

				if (spineObject.animations[anim].bones[bone].rotate) {
					if (!first) f.write(",");
					f.write("\n\t\t\t\t\t\t\"rotation\": [");
					for (var i in spineObject.animations[anim].bones[bone].rotate) {
						var keyframe = spineObject.animations[anim].bones[bone].rotate[i];
						f.write("\n\t\t\t\t\t\t\t{");
						f.write(util.format("\"time\": %d", keyframe.time));
						f.write(util.format(", \"value\": %d", keyframe.angle));
						if (keyframe.curve) {
							if (typeof keyframe.curve == "string")
								f.write(util.format(", \"curve\": \"%s\"", keyframe.curve.toUpperCase()));
							else
								f.write(util.format(", \"curve\": [%s, %s, %s, %s]", keyframe.curve[0], keyframe.curve[1], keyframe.curve[2], keyframe.curve[3]));
						}
						f.write("}");
						if (i < spineObject.animations[anim].bones[bone].rotate.length - 1) f.write(",");
					}
					f.write("\n\t\t\t\t\t\t]");
					first = false;
				}

				if (spineObject.animations[anim].bones[bone].scale) {
					if (!first) f.write(",");
					f.write("\n\t\t\t\t\t\"scale\": [");
					for (var i in spineObject.animations[anim].bones[bone].scale) {
						var keyframe = spineObject.animations[anim].bones[bone].scale[i];
						f.write("\n\t\t\t\t\t\t\t{");
						f.write(util.format("\"time\": %d", keyframe.time));
						f.write(util.format(", \"value\": [%d, %d]", keyframe.x ? keyframe.x : 0.0, keyframe.y ? keyframe.y : 0.0));
						if (keyframe.curve) {
							if (typeof keyframe.curve == "string")
								f.write(util.format(", \"curve\": \"%s\"", keyframe.curve.toUpperCase()));
							else
								f.write(util.format(", \"curve\": [%s, %s, %s, %s]", keyframe.curve[0], keyframe.curve[1], keyframe.curve[2], keyframe.curve[3]));
						}
						f.write("}");
						if (i < spineObject.animations[anim].bones[bone].scale.length - 1) f.write(",");
					}
					f.write("\n\t\t\t\t\t\t]");
				}
				f.write("\n\t\t\t\t\t}");
				if (boneIndex < numBones - 1)
					f.write(",");
				boneIndex++;
			}
			f.write("\n\t\t\t\t}");
		}


		if (spineObject.animations[anim].slots) {
			if (spineObject.animations[anim].bones) f.write(",");
			f.write("\n\t\t\t\t\"sprite\": {");

			var slotIndex = 0;
			var numSlots = Object.keys(spineObject.animations[anim].slots).length;
			for (var slot in spineObject.animations[anim].slots) {
				f.write(util.format("\n\t\t\t\t\t\"%s\": {", sanitizeKey(slot)));

				var first = true;
				if (spineObject.animations[anim].slots[slot].attachment) {
					f.write("\n\t\t\t\t\t\t\"frame\": [");
					for (var i in spineObject.animations[anim].slots[slot].attachment) {
						var keyframe = spineObject.animations[anim].slots[slot].attachment[i];
						f.write("\n\t\t\t\t\t\t\t{");
						f.write(util.format("\"time\": %d", keyframe.time));
						f.write(util.format(", \"value\": \"%s\"", keyframe.name));
						f.write("}");
						if (i < spineObject.animations[anim].slots[slot].attachment.length - 1) f.write(",");
					}
					f.write("\n\t\t\t\t\t\t]");
					first = false;
				}

				if (spineObject.animations[anim].slots[slot].color) {
					if (!first) f.write(",");
					f.write("\n\t\t\t\t\t\t\"rotation\": [");
					for (var i in spineObject.animations[anim].slots[slot].color) {
						var keyframe = spineObject.animations[anim].slots[slot].color[i];
						f.write("\n\t\t\t\t\t\t\t{");
						f.write(util.format("\"time\": %d", keyframe.time));
						f.write(util.format(", \"value\": %d", keyframe.color));
						if (keyframe.curve) {
							if (typeof keyframe.curve == "string")
								f.write(util.format(", \"curve\": \"%s\"", keyframe.curve.toUpperCase()));
							else
								f.write(util.format(", \"curve\": [%s, %s, %s, %s]", keyframe.curve[0], keyframe.curve[1], keyframe.curve[2], keyframe.curve[3]));
						}
						f.write("}");
						if (i < spineObject.animations[anim].slots[slot].color.length - 1) f.write(",");
					}
					f.write("\n\t\t\t\t\t\t]");
					first = false;
				}
				f.write("\n\t\t\t\t\t}");
			}
			f.write("\n\t\t\t\t}");
		}

		f.write("\n\t\t\t}");
		if (animIndex < numAnims - 1)
			f.write(",");
		animIndex++;
	}

	f.write("\n\t\t}");
	f.write("\n\t}");
	f.write("\n}");
	f.end("\n}");
					
	$("#spine_file").text("");
	$("#resource_name").val("");
	hide_form(); 	
		
	var msg = "<p class=\"text-success\" >PGI created.</p>";	
	$('#main_message').empty();
	$('#main_message').append(msg);
}


function submit() {
	var outputName = $("#resource_name").val();
	if(!outputName){
		alert("Please input a resource name.");
		return;
	}
	
    var spinePath = path.normalize($("#spine_file").text());
	
    fs.readFile(spinePath, 'utf8', function (err, data) {
        if (err) return alert(err);

		var outputPath = path.normalize(source_path + "/" + outputName + ".pgi");
		
		try{
			generate_pgi(data, outputName, outputPath);
		} 
		catch(e){			
			var re = new RegExp(String.fromCharCode(10), 'g');
			var stack = e.stack.replace(re, "</p><p class=\"text-alert\">");
			var msg = "<p class=\"text-alert\">" + e.message + "</p><p class=\"text-alert\">" + stack + "</p>";
			$('#message').empty();
			$('#config_message').append(msg);
			
			if (fs.existsSync(outputPath)) { fs.unlinkSync(outputPath); }
		}
	});
}



//-------------------------------------------------------------------------------------------------------------------------

function resize_bottom() {
    $('#config-bottom').width($('.content').width() - 2);
}


function initialize() {
    $("#config").hide();
	
    // prevent default behavior from changing page on dropped file
    window.ondragover = function (e) { e.preventDefault(); return false };
    window.ondrop = function (e) { e.preventDefault(); return false };

    $(window).resize(resize_bottom);

    $("#holder")
	.on("dragover", function () { this.className = 'hover'; return false; })
	.on("dragleave", function () { this.className = ''; return false; })
	.on("drop", function (e) {
	    this.className = '';
	    e.preventDefault();
	    load_form(e.originalEvent.dataTransfer.files[0].path);
	    return false;
	});

    $("#cancel").click(function () { hide_form(); return false; });
    $("#submit").click(function () { submit(); return false; });
}


function load_form(file) {
    $("#holder").hide();
    resize_bottom();
    $("#config").show();
    $("#spine_file").text(file);
}

//-------------------------------------------------------------------------------------------------------------------------------