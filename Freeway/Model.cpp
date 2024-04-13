#include "Model.h"


Model::Model() : cur_road(Road()),
				 ticks_to_next_auto(0),
				 min_speed(CONST_MIN_SPEED),
				 max_speed(CONST_MAX_SPEED),
				 min_time(CONST_MIN_TIME),
				 max_time(CONST_MAX_TIME),
				 coef_acceleration(CONST_COEF_ACCELERATION),
				 coef_slowdown(CONST_COEF_SLOWDOWN) {}

void
Model::tick() {
	if (ticks_to_next_auto > max_time * TICKS_IN_SEC) {
		ticks_to_next_auto = max_time * TICKS_IN_SEC;
	}
	if (ticks_to_next_auto)
		--ticks_to_next_auto;
	if (ticks_to_next_auto <= 0 && cur_road.free_road()) {
		ticks_to_next_auto = (min_time + rand() % (max_time - min_time + 1)) * TICKS_IN_SEC;
		cur_road.add_auto(min_speed + rand() % (max_speed - min_speed + 1), coef_acceleration, coef_slowdown);
	}
	cur_road.tick();
}

void
Model::set_params(int min_ti, int max_ti, int min_sp, int max_sp, float coef_acc, float coef_slow) {
	if (min_ti > 0 && max_ti >= min_ti && min_sp > 0 && max_sp >= min_sp && coef_acc > 0 && coef_slow) {
		min_time = min_ti;
		max_time = max_ti;
		min_speed = min_sp;
		max_speed = max_sp;
		coef_slowdown = coef_slow * CONST_COEF_SLOWDOWN;
		coef_acceleration = coef_acc * CONST_COEF_ACCELERATION;
	}
}

Road&
Model::get_road() {
	return cur_road;
}






void
Road::tick() {
	for (auto it_auto = autos.begin(); it_auto != autos.end(); ) {
		if (it_auto->tick()) {
			++it_auto;
		}
		else {
			it_auto = autos.erase(it_auto);
			if (it_auto != autos.end()) {
				it_auto->next_auto_left();
			}
		}
	}
}

void
Road::add_auto(double init_speed, double coef_acc, double coef_slow) {
	Auto* next_auto = NULL;
	if (!autos.empty()) {
		next_auto = &autos.back();
	}
	autos.push_back(Auto(init_speed, next_auto, coef_acc, coef_slow));
}

std::list<Auto>&
Road::get_list_auto() {
	return autos;
}

bool
Road::free_road() {
	return autos.empty() ? true : autos.back().get_coord() > 3 * LEN_AUTO;
}






Auto::Auto(double init_speed, Auto* next_auto, double coef_acc, double coef_slow) :
	status(CONSTANT_SPEED),
	initial_speed(init_speed),
	cur_speed(init_speed),
	need_speed(-1),
	coef_acceleration(coef_acc),
	coef_slowdown(coef_slow),
	ticks_with_need_speed(0),
	coord(0),
	next_auto(next_auto)
{}

bool
Auto::tick() {
	double distance_to_next_auto = LEN_ROAD, speed_next_auto, coord_next_auto;
	if (next_auto != NULL) {
		coord_next_auto = next_auto->get_coord();
		distance_to_next_auto = coord_next_auto - coord;
		speed_next_auto = next_auto->get_speed();
	}
	if (ticks_with_need_speed && cur_speed > need_speed) {
		++ticks_with_need_speed;
		status = SLOWDOWN;
		compute_next_coord(need_speed);
	} else if (distance_to_next_auto < 3 * LEN_AUTO) {
		if (speed_next_auto < cur_speed) {
			status = SLOWDOWN;
		}
		else if (cur_speed > 0) {
			cur_speed = speed_next_auto;
			status = CONSTANT_SPEED;
		}
		compute_next_coord(speed_next_auto);
	}
	else {
		if (ticks_with_need_speed == 0 && cur_speed < initial_speed) {
			status = ACCELERATION;
		}
		compute_next_coord(need_speed);
	}
	if (ticks_with_need_speed) {
		--ticks_with_need_speed;
	}
	if (next_auto != NULL) {
		if (coord_next_auto - coord < LEN_AUTO) {
			crash();
			next_auto->crash();
			coord = coord_next_auto - LEN_AUTO - 0.1;
		}
	}
	if (coord > LEN_ROAD) {
		return false;
	}
	return true;
}

void
Auto::crash() {
	status = CRASH;
	cur_speed = 0;
	need_speed = 0;
	ticks_with_need_speed = TIME_CRASH * TICKS_IN_SEC;
}

void
Auto::artificial_delay(int speed, int time) {
	need_speed = speed;
	ticks_with_need_speed = time * TICKS_IN_SEC;
}

double
Auto::get_coord() const {
	return coord;
}

double
Auto::get_speed() const {
	return cur_speed;
}

StatusAuto
Auto::get_status() const {
	return status;
}

void
Auto::next_auto_left() {
	next_auto = NULL;
}

void
Auto::compute_next_coord(double speed_next_auto) {
	double time = 1.0 / TICKS_IN_SEC;
	coord += cur_speed * time;
	if (status == ACCELERATION)  {
		if (cur_speed + coef_acceleration * time > initial_speed) {
			time = (initial_speed - cur_speed) / coef_acceleration;
			coord += (initial_speed - cur_speed) * (1.0 / TICKS_IN_SEC - time);
			cur_speed = initial_speed;
			status = CONSTANT_SPEED;
		}
		else {
			cur_speed += coef_acceleration * time;
		}
		coord += coef_acceleration * time * time / 2;
	}
	else if (status == SLOWDOWN) {
		//speed_next_auto = std::min(0.0, speed_next_auto);
		if (cur_speed + coef_slowdown * time < speed_next_auto) {
			time = (speed_next_auto - cur_speed) / coef_slowdown;
			coord += (speed_next_auto - cur_speed) * (1.0 / TICKS_IN_SEC - time);
			cur_speed = speed_next_auto;
			status = CONSTANT_SPEED;
		}
		else {
			cur_speed += coef_slowdown * time;
		}
		coord += coef_slowdown * time * time / 2;
	}
}
