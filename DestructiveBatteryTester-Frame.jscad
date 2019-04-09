// Destructive Battery Tester Frame
// https://github.com/RainerU/DestructiveBatteryTester


const hx = 17; // holder x
const hy = 58; // holder y
const hbs = 3; // holder bottom strength
const hbh = 0.8; // holder bottom hole additional depth
const hz = 5; // inserting height

const hh = 10; // middle hole diam
const hs = 3; // middle hole screw diam

const hd = 5; // distance
const hn = 8; // number of holders

const bx = hd + hn*hx + (hn-1)*hd + hd; // base x
const by = hd + hy + hd; // base y
const bz = hbs + hz; // base z

const res = 32;

function base() {
	var base = CSG.cube({
		corner1: [0, 0, 0],
		corner2: [bx, by, bz],
	});
	
	return base;
}

function holder() {
	// block
	var holder = CSG.cube({
		corner1: [0, 0, hbs],
		corner2: [hx, hy, hbs + hz],
	})
	
	// middle hole
	holder = holder.union(
		CSG.cylinder({
			start: [hx/2, hy/2, hbs - hbh],
			end:   [hx/2, hy/2, hbs + hz],
			radius: hh/2,
			resolution: res,
		})
	);
	holder = holder.union(
		CSG.cylinder({
			start: [hx/2, hy/2, 0],
			end:   [hx/2, hy/2, hbs + hz],
			radius: hs/2,
			resolution: res,
		})
	);
	
	// cabel channel
	holder = holder.union(
		CSG.cube({
			corner1: [hx - 4, -2, hbs - 2],
			corner2: [hx - 2, 0, hbs + hz],
		})
	);
	holder = holder.union(
		CSG.cube({
			corner1: [hx - 4, hy, hbs - 2],
			corner2: [hx - 2, hy + 2, hbs + hz],
		})
	);
	holder = holder.union(
		CSG.cube({
			corner1: [hx - 4, -hd, hbs - 2],
			corner2: [hx - 2, hy+2, hbs + 2],
		})
	);
	
	
	return holder;
}

function main() {
	var main = base();
	
	for (var i=0; i<hn; i++) {
		main = main.subtract(holder().translate([hd + i*(hx + hd), hd, 0]));
	}
	
	return main;
}

