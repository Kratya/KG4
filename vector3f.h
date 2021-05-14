#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include "glut.h"
#include <algorithm> 
#include <vector>
#include<algorithm>
#include <stdio.h>
#include <sstream>

class Vector3f {
public:
	double x, y, z;
	Vector3f() {};
	Vector3f(double _x, double _y, double _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3f& operator=(const Vector3f& right) {
		//проверка на самоприсваивание
		if (this == &right) {
			return *this;
		}
		this->x = right.x;
		this->y = right.y;
		this->z = right.z;
		return *this;
	}

	//Переопределение оператора +
	Vector3f operator + (Vector3f _Vector) {
		return Vector3f(_Vector.x + x, _Vector.y + y, _Vector.z + z);
	}

	//Переопределение оператора -
	Vector3f operator - (Vector3f _Vector) {
		return Vector3f(x - _Vector.x, y - _Vector.y, z - _Vector.z);
	}

	//Переопределение оператора *
	Vector3f operator * (double num) {
		return Vector3f(x * num, y * num, z * num);
	}

	//Переопределение оператора /
	Vector3f operator / (double num) {
		return Vector3f(x / num, y / num, z / num);
	}
};
