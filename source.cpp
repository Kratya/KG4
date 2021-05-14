#define _CRT_SECURE_NO_WARNINGS
#include "vector3f.h"

using namespace std;

#define ESCAPE '\033'
#define DELETE '\010'

unsigned char* image1;

int J = 0, JL = 0;

int Width = 700, Height = 700;
double t_min = 1, t_max = 1e+25;
double tetta = 120;
double N = 1;
double Aspect = (double)Width / Height;
double H = N * tan((double)tetta * 3.1415926535897932 / 360.0);
double W = H * Aspect;

int figMenu, mainMenu, lightMenu;

Vector3f O(0, 0, -150);
Vector3f D, pointHit;

int r, c;
int numFigures, numSpheres = 0;

int depth = 1;

vector<int> flagFigures;
vector<int> flagLight{ 1,1,1,1 };

struct Material {
	Material(const double& ka, const double& kd, const double& ks, const double& kr, const Vector3f& color, const double& spec) :ka(ka), kd(kd), ks(ks), kr(kr), Color(color), specularExponent(spec) {}
	Material() : ka(1), kd(1), ks(0), kr(0), Color(), specularExponent() {}
	double ka;// фоновое 
	double kd;// диффузное
	double ks;//блики
	double kr;//отражение
	Vector3f Color;
	double specularExponent;
};

//список материалов
vector<Material> mats{ Material(0.9, 0.7,0.5, 0.5, Vector3f(0.,1.,0.), 50.),Material(0.9, 0.7, 2., 0.5, Vector3f(0.,0.,1.), 50.),Material(0.9, 0.7, 2., 0.5, Vector3f(1.,1.,0.), 50.) };

struct Sphere {
	Vector3f center;
	double radius;
	Material material;
	Sphere(Vector3f center, double radius, int numM) :center(center), radius(radius), material(mats[numM]) {}
	Sphere() {}
};

struct Tetra {
	vector<Vector3f> points{ Vector3f(0,0,0),Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(0,0,0) };
	vector<Vector3f> normals{ Vector3f(0,0,0),Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(0,0,0) };
	vector<double> di{ 0,0,0,0 };
	Material material;
	Tetra() {}
};

struct Plane {
	Vector3f normal;
	double di;
	Material material;
	Plane(Vector3f normal, double di, Material material) : normal(normal), di(di), material(material) {}
};

struct Light {
	Light(const Vector3f& p, const double& i) : position(p), intensity(i) {}
	Vector3f position;
	double intensity;
};

Vector3f tmpCol;
vector<Sphere> spheres;
vector<Tetra> tetras;
Sphere closestSphere;
Tetra closestTetra;

Plane plane(Vector3f(0., -4, 1.), 30., Material(1., 0.4, 0., 0., Vector3f(0.7, 0.7, 0.7), 0.));
Vector3f nowColor{ 1.,1.,1. };
Vector3f backGroudColor{ 1.,1.,1. };
int closestFlag;//ближайший объект: 0-сфера, 1-тетраэдр
double closestT;

vector<Light>  lights{ Light(Vector3f(-25,20,0), 1), Light(Vector3f(130, 0, 10), 0.5) };

//норма вектора
double Norm(Vector3f v1) {
	return (double)sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
}

//нормализация
Vector3f Normalize(Vector3f v1) {
	double norm = Norm(v1);
	Vector3f res;
	res.x = 0;
	res.y = 0;
	res.z = 0;
	if (norm != 0) {
		res.x = v1.x / norm;
		res.y = v1.y / norm;
		res.z = v1.z / norm;
	}
	return res;
}

//векторное произведение 
Vector3f Vector(Vector3f v1, Vector3f v2) {
	Vector3f res;
	res.x = v1.y * v2.z - v2.y * v1.z;
	res.y = -v1.x * v2.z + v2.x * v1.z;
	res.z = v1.x * v2.y - v2.x * v1.y;
	return res;
}

//нормаль между двумя векторами
Vector3f Normal(Vector3f v1, Vector3f v2) {
	Vector3f res = Vector(v1, v2);
	res = Normalize(res);
	return res;
}

double dot(Vector3f vec1, Vector3f vec2)
{
	return (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z);
}

Vector3f Reflect(Vector3f& L, Vector3f& N) {  //отраженный луч h
	return L - Vector((Vector(L, N) * 2), N);
}

void ReadFigures() {
	ifstream fin("figures.txt");
	fin >> numFigures;
	flagFigures.resize(numFigures);
	fill(flagFigures.begin(), flagFigures.end(), 1);
	int type;// 0-сфера,1-пирамида
	for (int i = 0; i < numFigures; i++)
	{
		fin >> type;
		if (type == 0) {
			int numM;
			Sphere sphere;
			fin >> sphere.center.x >> sphere.center.y >> sphere.center.z >> sphere.radius >> numM;
			sphere.material = mats[numM];
			spheres.push_back(sphere);
			numSpheres++;
		}
		else {
			int numM;
			Tetra tetraedr;
			for (int i = 0; i < 4; i++) {
				fin >> tetraedr.points[i].x >> tetraedr.points[i].y >> tetraedr.points[i].z;
			}

			fin >> numM;

			//расчет нормалей и di
			tetraedr.normals[0] = Normal(tetraedr.points[1] - tetraedr.points[0], tetraedr.points[2] - tetraedr.points[1]);
			tetraedr.normals[1] = Normal(tetraedr.points[0] - tetraedr.points[1], tetraedr.points[3] - tetraedr.points[0]);
			tetraedr.normals[2] = Normal(tetraedr.points[1] - tetraedr.points[2], tetraedr.points[3] - tetraedr.points[1]);
			tetraedr.normals[3] = Normal(tetraedr.points[2] - tetraedr.points[0], tetraedr.points[3] - tetraedr.points[2]);

			for (int i = 0; i < 4; i++)
				tetraedr.di[i] = dot(tetraedr.points[i], tetraedr.normals[i]);
			tetraedr.material = mats[numM];


			tetras.push_back(tetraedr);
		}
	}
	fin.close();
}

Vector3f CanvasToViewport() {
	double uc = 2. * W * c / Width;
	double vr = 2. * H * r / Height;

	Vector3f u(1., 0, 0), v(0, 1., 0), w(0, 0, 1.);

	Vector3f tmp = u * uc + v * vr + w * N;
	return tmp;
}

void IntersectRaySphere(Vector3f orig, Vector3f dir, Sphere sphere, double& t1, double& t2) {
	Vector3f C = sphere.center;
	double rad = sphere.radius;
	Vector3f OC = orig - C;

	double k1 = dot(dir, dir);
	double k2 = 2 * dot(OC, dir);
	double k3 = dot(OC, OC) - rad * rad;

	double discriminant = k2 * k2 - 4 * k1 * k3;
	if (discriminant < 0)
		t1 = t2 = 1e+25;
	else
	{
		t1 = (-k2 + sqrt(discriminant)) / (2 * k1);
		t2 = (-k2 - sqrt(discriminant)) / (2 * k1);
	}
}

int IntersectRayTetra(Vector3f orig, Vector3f dir, Tetra tetraedr, double& alfa) {
	// Пересечение луча и многогранника:
	// Вход: Луч из точки O в направлении d (|d| = 1).
	// Набор k плоскостей с нормалями ni и параметрами di, i = 1, k.
	double fMax = 1;
	double bMin = 1e+25;
	alfa = 1e+25;
	for (int i = 0; i < 4; i++) {
		// Пересечение луча и плоскости
		double s = dot(dir, tetraedr.normals[i]);
		if (s == 0) { // луч и плоскость параллельны
			if (dot(orig, tetraedr.normals[i]) > tetraedr.di[i]) {
				return 1; // пересечения нет
			}
		}
		// Если непараллельно плоскости
		alfa = (tetraedr.di[i] - dot(orig, tetraedr.normals[i])) / s;
		if (dot(dir, tetraedr.normals[i]) < 0) { // если пересечение "сверху"
			if (alfa > fMax) {
				if (alfa > bMin) {
					return 1; // пересечения нет
				}
				fMax = alfa;
			}
		}
		else { // если пересечение "снизу"
			if (alfa < bMin) {
				if (alfa < 0 || alfa < fMax) {
					return 1; // пересечения нет
				}
				bMin = alfa;
			}
		}
	}
	alfa = fMax > 0 ? fMax : bMin;
	return 0;
}

int IntersectRayPlane(Vector3f orig, Vector3f dir, Plane plane, double& alfa) {

	alfa = 1e+25;

	// Вход: Луч из точки p в направлении u (|u| = 1).
	// Плоскость с нормалью n и параметром d.
	double s = dot(dir, plane.normal);

	if (s != 0) { // пересечения нет

		double tmp = (plane.di - dot(orig, plane.normal)) / s;
		if (tmp >= 0) {
			alfa = tmp;
			return 0;
		}
	}
	return 1;
}

int FindTetraPlane(Tetra tetraedr, Vector3f toFind) {

	//передняя
	if (toFind.x >= min(tetraedr.points[0].x, tetraedr.points[1].x) && toFind.x <= max(tetraedr.points[0].x, tetraedr.points[1].x) &&
		toFind.z >= min(tetraedr.points[0].z, tetraedr.points[1].z) && toFind.z <= max(tetraedr.points[0].z, tetraedr.points[1].z) &&
		toFind.y >= min(tetraedr.points[0].y, tetraedr.points[1].y) && toFind.y <= tetraedr.points[3].y)
		return 1;

	//правая
	if (toFind.x >= min(tetraedr.points[1].x, tetraedr.points[2].x) && toFind.x <= max(tetraedr.points[1].x, tetraedr.points[2].x) &&
		toFind.z >= min(tetraedr.points[1].z, tetraedr.points[2].z) && toFind.z <= max(tetraedr.points[1].z, tetraedr.points[2].z) &&
		toFind.y >= min(tetraedr.points[1].y, tetraedr.points[2].y) && toFind.y <= tetraedr.points[3].y)
		return 2;

	//левая
	if (toFind.x >= min(tetraedr.points[2].x, tetraedr.points[0].x) && toFind.x <= max(tetraedr.points[2].x, tetraedr.points[0].x) &&
		toFind.z >= min(tetraedr.points[2].z, tetraedr.points[0].z) && toFind.z <= max(tetraedr.points[2].z, tetraedr.points[0].z) &&
		toFind.y >= min(tetraedr.points[2].y, tetraedr.points[0].y) && toFind.y <= tetraedr.points[3].y)
		return 3;

	return 0;
}

Vector3f TraceRay(Vector3f& orig, Vector3f& dir, int depth) {
	closestFlag = -1;
	closestT = 1e+25;
	double sphT = 1e+25, pirT = 1e+25, plT = 1e+25;

	//Material closestMaterial;
	bool flagS = false, flagP = false, flagPl = false;
	Vector3f curColor = backGroudColor;

	for (int i = 0; i < numSpheres; i++) {
		if (flagFigures[i] == 1) {
			Sphere sphere = spheres[i];
			double t1, t2;
			IntersectRaySphere(orig, dir, sphere, t1, t2);
			if (t1 > t_min && t1 < t_max && t1 < sphT) {
				flagS = true;
				sphT = t1;
				closestSphere = sphere;
			}
			if (t2 > t_min && t2 < t_max && t2 < sphT) {
				flagS = true;
				sphT = t2;
				closestSphere = sphere;
			}
		}
	}

	for (int i = numSpheres; i < numFigures; i++) {
		if (flagFigures[i] == 1) {
			Tetra piramid = tetras[i - numSpheres];
			double alfa;
			if (IntersectRayTetra(orig, dir, piramid, alfa) == 0) {
				if (alfa > t_min && alfa < t_max && alfa < pirT) {
					flagP = true;
					pirT = alfa;
					closestTetra = piramid;
				}
			}
		}
	}

	double alfa;
	if (IntersectRayPlane(orig, dir, plane, alfa) == 0) {
		if (alfa > t_min && alfa < t_max && alfa < plT) {
			flagPl = true;
			plT = alfa;
		}
	}


	if (flagPl == true) {//нашли плоскость
		if (flagS == true) {//нашли плоскость, сферу и хз
			if (flagP == true) {//нашли все 3
				if (pirT < sphT) {
					if (plT < pirT) {//плоскость ближе
						curColor = plane.material.Color;
						closestT = plT;
						closestFlag = 2;
					}
					else {//тетраэдр ближе
						curColor = closestTetra.material.Color;
						closestT = pirT;
						closestFlag = 1;
					}
				}
				else {//проверим плоскость и сферу
					if (plT < sphT) {//плоскость ближе
						curColor = plane.material.Color;
						closestT = plT;
						closestFlag = 2;
					}
					else {//сфера ближе
						curColor = closestSphere.material.Color;
						closestT = sphT;
						closestFlag = 0;
					}
				}
			}
			else {//нашли сферу и плоскость
				if (sphT < plT) {
					curColor = closestSphere.material.Color;
					closestT = sphT;

					closestFlag = 0;
				}
				else {
					curColor = plane.material.Color;
					closestT = plT;

					closestFlag = 2;
				}
			}
		}
		else {//нашли плоскость и пирамиду
			if (pirT < plT) {
				curColor = closestTetra.material.Color;
				closestT = pirT;

				closestFlag = 1;
			}
			else {
				curColor = plane.material.Color;
				closestT = plT;

				closestFlag = 2;
			}
		}

	}

	else {
		if (flagS == true) {
			if (flagP == true) {
				if (pirT < sphT) {
					curColor = closestTetra.material.Color;
					closestT = pirT;

					closestFlag = 1;
				}
				else {
					curColor = closestSphere.material.Color;
					closestT = sphT;
					closestFlag = 0;
				}
			}
			else {//нашли только сферу
				curColor = closestSphere.material.Color;
				closestT = sphT;
				closestFlag = 0;
			}
		}
		else {
			if (flagP == true) {//нашли только пирамиду
				curColor = closestTetra.material.Color;
				closestT = pirT;
				closestFlag = 1;
			}
		}

	}


	//ЦВЕТА и СВЕТ
	double diffuseLightIntensity = 0, specularLightIntensity = 0, ambientLigthIntensity = 0.5;

	if (closestFlag == 0) {//свет для сферы
		pointHit = orig + dir * closestT;
		curColor = closestSphere.material.Color;

		//double ka, kd, ks, specExp;

		Vector3f ReflectColor = Vector3f(1., 0., 0.);
		Vector3f ReflectDir;
		Vector3f ReflectOrig;

		for (int i = 0; i < lights.size(); i++) {
			if (flagLight[i] == 1) {
				Vector3f lightDir = Normalize(lights[i].position - pointHit);//падающий луч l
				Vector3f PointToCenter = Normalize(pointHit - closestSphere.center); //нормаль в обратную сторону n
				ambientLigthIntensity *= closestSphere.material.ka;
				diffuseLightIntensity += lights[i].intensity * max(0., dot(lightDir, PointToCenter)) * closestSphere.material.kd;
				specularLightIntensity += powf(max(0., (dot(PointToCenter, Reflect(lightDir, PointToCenter)))), closestSphere.material.specularExponent) * lights[i].intensity * closestSphere.material.ks;
			}
		}
		curColor = curColor * ambientLigthIntensity + curColor * diffuseLightIntensity + Vector3f(1., 1., 1.) * specularLightIntensity;
	}

	else {
		if (closestFlag == 1) {//свет для пирамиды
			pointHit = orig + dir * closestT;
			curColor = closestTetra.material.Color;

			for (int i = 0; i < lights.size(); i++) {
				if (flagLight[i] == 1) {
					Vector3f lightDir = Normalize(lights[i].position - pointHit);
					int numNormal = FindTetraPlane(closestTetra, pointHit);
					Vector3f PointToCenter = closestTetra.normals[numNormal];
					diffuseLightIntensity += lights[i].intensity * max(0., dot(lightDir, PointToCenter));
					specularLightIntensity += powf(max(0., (dot(PointToCenter, Reflect(lightDir, PointToCenter)))), closestTetra.material.specularExponent) * lights[i].intensity * closestTetra.material.ks;
				}
			}
			curColor = curColor * ambientLigthIntensity + curColor * diffuseLightIntensity + Vector3f(1., 1., 1.) * specularLightIntensity;
		}
		else if (closestFlag == 2) {//свет для плоскости
			Vector3f ReflectDir, ReflectOrig, ReflectColor;

			if (closestFlag == 2) {//свет для плоскости
				curColor = plane.material.Color;

				pointHit = orig + dir * closestT;
				Vector3f PointToCenter = plane.normal * (-1);

				for (int i = 0; i < lights.size(); i++) {
					if (flagLight[i] == 1) {
						Vector3f lightDir = Normalize(lights[i].position - pointHit);

						ambientLigthIntensity *= plane.material.ka;
						diffuseLightIntensity += lights[i].intensity * max(0., dot(lightDir, PointToCenter)) * plane.material.kd;
						specularLightIntensity += powf(max(0., (dot(PointToCenter, Reflect(lightDir, PointToCenter)))), plane.material.specularExponent) * lights[i].intensity * plane.material.ks;
					}
				}
				curColor = curColor * ambientLigthIntensity + curColor * diffuseLightIntensity + Vector3f(1., 1., 1.) * specularLightIntensity;
			}
		}
	}

	return curColor;
}

/* Функция вывода на экран */
void Display(void) {
	glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	for (r = -Height / 2; r <= Height / 2; r++) {
		for (c = -Width / 2; c < Width / 2; c++) {

			if (c == 0 && r == 0)
			{
				cout << "*";
			}
			D = CanvasToViewport();//направление
			nowColor = TraceRay(O, D, 0);// возвращаем цвет пикселя
			glBegin(GL_POINTS);
			glColor3f(nowColor.x, nowColor.y, nowColor.z);
			glVertex2f(c, r);
			glEnd();
		}
	}

	glFinish();
}

/* Функция обработки сообщений от клавиатуры */
void Keyboard(unsigned char key, int x, int y) {
	if (key == 'a') { // переходим на свет влево
		if (JL > 0)
			JL--;
	}
	if (key == 'd') { // переходим на свет вправо
		if (JL < lights.size() - 1)
			JL++;
	}
	if (key == 'w') {
		flagLight[JL] = 1; // вкл
		glutPostRedisplay();
	}
	if (key == 's') {
		flagLight[JL] = 0; // не вкл
		glutPostRedisplay();
	}

	if (key == '-') { // переходим на фигуру влево
		if (J > 0)
			J--;
	}
	if (key == '=') { // переходим на фигуру вправо
		if (J < numFigures - 1)
			J++;
	}
	if (key == 'p') {
		flagFigures[J] = 1; // рисуем
		glutPostRedisplay();
	}
	if (key == 'o') {
		flagFigures[J] = 0; // не рисуем
		glutPostRedisplay();
	}
}

int menuFlag;

void processMainMenu(int option) {
	switch (option) {
	case 1: 
		if (flagFigures[J] == 1)
			flagFigures[J] = 0;
		else flagFigures[J] = 1;
		break;
	case 2: 
		if (flagLight[JL] == 1)
			flagLight[JL] = 0;
		else flagLight[JL] = 1;
	}
	glutPostRedisplay();
}

void processFigMenu(int option) {
	switch (option) {
	case 1:
		if (J < numFigures - 1)
			J++;
		break;
	case 2:
		if (J > 0)
			J--;
		break;
	}
	glutPostRedisplay();
}
void processLightMenu(int option) {
	switch (option) {
	case 1:
		if (JL < lights.size() - 1)
			JL++;
		break;
	case 2:
		if (JL > 0)
			JL--;
		break;
	}
	glutPostRedisplay();
}
void createPopupMenus() {

	figMenu = glutCreateMenu(processFigMenu);
	glutAddMenuEntry("Следующая фигура", 1);
	glutAddMenuEntry("Предыдущая фигура", 2);

	lightMenu = glutCreateMenu(processLightMenu);
	glutAddMenuEntry("Следующий источник", 1);
	glutAddMenuEntry("Предыдущий источник", 2);

	mainMenu = glutCreateMenu(processMainMenu);

	glutAddMenuEntry("Включить/Выключить фигуру", 1);
	glutAddSubMenu("Переключение фигуры", figMenu);
	glutAddMenuEntry("Включить/Выключить свет", 2);
	glutAddSubMenu("Переключение света", lightMenu);

	// прикрепить меню к правой кнопке мыши
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/* Функция изменения размеров окна */
void Reshape(GLint w, GLint h) {
	Width = w;
	Height = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-w / 2, w / 2, -h / 2, h / 2, -10000, 10000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/* Головная программа */
void main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(Width, Height);
	ReadFigures();
	glutCreateWindow("RayTracing");
	glutKeyboardFunc(Keyboard);
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	createPopupMenus();
	glutMainLoop();
}