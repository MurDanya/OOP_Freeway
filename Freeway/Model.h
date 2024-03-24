#pragma once
#include <list>

// Внутренние параметры модели по умолчанию
enum {
	CONST_MIN_SPEED = 40,
	CONST_MAX_SPEED = 100,
	CONST_MIN_TIME = 1,
	CONST_MAX_TIME = 5,
	CONST_COEF_ACCELERATION = 10,
	CONST_COEF_SLOWDOWN = -30,
	LEN_AUTO = 18,
	LEN_ROAD = 1000,
	TICKS_IN_SEC = 60,
	TIME_CRASH = 3,
};

// Состояние Автомобиля
enum StatusAuto {
	CONSTANT_SPEED,
	ACCELERATION,
	SLOWDOWN,
	CRASH
};

class Model;


class Auto {
public:
	Auto(double init_speed, Auto* next_auto, Model* mod);
	bool tick();
	void crash();
	void artificial_delay(int, int);
	double get_coord() const;
	double get_speed() const;
	StatusAuto get_status() const;
	void next_auto_left();
private:
	Model* cur_model;
	StatusAuto status;
	double initial_speed, cur_speed, need_speed;
	int ticks_with_need_speed;
	double coord;
	Auto* next_auto;
	void compute_next_coord(double);
};


class Road {
public:
	Road() {}
	void tick();
	void add_auto(double init_spped, Model* mod);
	bool free_road();
	std::list<Auto>& get_list_auto();
private:
	std::list<Auto> autos;
};


class Model {
public:
	Model();
	void tick();
	double get_coef_acc();
	double get_coef_slow();
	void set_params(int, int, int, int, float, float);
	Road& get_road();
private:
	Road cur_road;
	int ticks_to_next_auto;
	int min_speed, max_speed, min_time, max_time;
	double coef_acceleration, coef_slowdown;
};
