#include <iostream>
#include <vector>
#include <unordered_set>
#include <math.h>   
#include <unordered_map>
#include <fstream>
#include <ctime>
#include <string>
#include <windows.h>
using namespace std;


// variables globales
const int N = 5;
unsigned int circuitos_creados = 0;
unsigned int combinaciones_totales = 0;
unsigned int combinaciones_descartadas = 0;
unsigned int maxTam;
string nombreFicheroInfo;
string nombreFicheroFun;

vector<unsigned int> v_nivel_0;
unordered_set<unsigned int> s_nivel_0;
// archivo
ofstream fs;
ofstream fs_ft;
// mapa que asigna a cada funcion el tamaño minimo del circuito que la computa
unordered_map<unsigned int, short> func_tam;

/*
informacion de las entradas de una puerta
*/
struct info {
	unsigned int funcion; // funcion que computan las entradas
	bool hayPorUsar; // una de las entradas pertenece a por usar
	unsigned int porUsar; // si pertenence, cuál es
	vector<unsigned int> usadosNivelAnt; // entradas usadas del nivel anterior
};

/*
calcula los huecos existentes sobre ese nivel (en el caso de que usemos solo una vez cada puerta
de los niveles superiores)
*/
vector<int> calcula_vacios(const vector<int>& estructura) {
	vector<int> huecos(estructura.size(), 0);
	for (int i = estructura.size() - 2; i > 0; i--) {
		if (estructura[i + 1] >= estructura[i])
			huecos[i] = huecos[i + 1] + estructura[i + 1];
		else
			huecos[i] = huecos[i + 1] + 2 * estructura[i + 1] - estructura[i];
	}
	huecos[0] = -1;
	return huecos;
}

/*
genera las estructuras para un tamño a partir de las del tamaño anterior
*/
vector<vector<int>> genera_estructuras(int n, vector<vector<int>>& estructuras_anteriores) {
	vector<vector<int>> estructuras_nivel_n;
	for (vector<int> estructura : estructuras_anteriores) {
		// hijo
		vector<int> v = { 1 };
		v.insert(v.end(), estructura.begin(), estructura.end());
		estructuras_nivel_n.push_back(v);

		//hermano
		vector<int> h = calcula_vacios(estructura);
		if ((estructura.size() > 1) && (estructura[0] < 2 * estructura[1] + h[1])) {
			estructura[0]++;
			estructuras_nivel_n.push_back(estructura);
		}
	}
	return estructuras_nivel_n;
}

/*
calcula nCk
*/
unsigned int nChoosek(unsigned int n, unsigned int k)
{
	if (k > n) return 0;
	if (k * 2 > n) k = n - k;
	if (k == 0) return 1;

	int result = n;
	for (int i = 2; i <= k; ++i) {
		result *= (n - i + 1);
		result /= i;
	}
	return result;
}

/*
comprueba la condicin 5 de la definición de estructura válida
*/
bool check_condicion5(const vector<int>& estructura) {
	if (estructura[0] > nChoosek(N, 2) + N)
		return false;

	int ant = N + 1;
	for (int i = 1; i < estructura.size(); i++) {
		if (estructura[i] > estructura[i - 1] * ant + nChoosek(estructura[i - 1], 2))
			return false;
		ant += estructura[i - 1];
	}

	return true;
}

/*
genera combinaciones sin repetición de un tamaño (su uso concreto es dado el tamaño de ese nivel)
*/
void generaCombinaciones(vector<info> arr, int n, int r, int index, vector<info> data, int i, vector<vector<info>>& sol)
{
	// Combinación lista
	if (index == r)
	{
		sol.push_back(data);
		return;
	}

	// No quedan elementos
	if (i >= n)
		return;

	// incluir actual
	data[index] = arr[i];
	generaCombinaciones(arr, n, r, index + 1, data, i + 1, sol);

	// cambiar actual por siguiente
	generaCombinaciones(arr, n, r, index, data, i + 1, sol);
}

/*
dadas dos funciones, devuelve su NAND
*/
unsigned int funcion_computa(unsigned int e1, unsigned int e2) {
	return ~(e1 & e2);
}

/*
dadas dos entradas, computa la función y la anota en el mapa
*/
void anota_funcion(const unsigned int e1, const unsigned int e2, const int tam) {
	unsigned int funcionComputa = funcion_computa(e1, e2);
	++circuitos_creados;
	if (func_tam.count(funcionComputa) == 0) {
		func_tam[funcionComputa] = tam;
		fs_ft << funcionComputa << " " << tam << "\n";
	}
}


void genera_circuitos(const vector<int>& estructura, const vector<int>& vacios, unordered_set<unsigned int>& por_usar, const int nivel_actual, vector<vector<unsigned int>>& niveles_anteriores, const int tam, unordered_set <unsigned int>& funciones_usadas, int puertas_niv_ant) {
	// último nivel
	if (nivel_actual == estructura.size()) {
		vector<unsigned int> nivel_anterior = niveles_anteriores[nivel_actual - 1];
		unsigned int e1 = nivel_anterior[0];

		// hay 2 en el nivel anterior -> juntarlos
		if (nivel_anterior.size() == 2) {
			unsigned int e2 = nivel_anterior[1];
			anota_funcion(e1, e2, tam);
		}

		// quedan por usar -> junto nivel anterior con el que queda por usar
		else if (!por_usar.empty()) {
			auto e2 = *por_usar.begin();
			anota_funcion(e1, e2, tam);
		}

		// resto de casos -> junto nivel anterior con todos los anteriores
		else {
			// ella con ella misma
			anota_funcion(e1, e1, tam);

			// con todas las anteriores
			for (int i = niveles_anteriores.size() - 2; i >= 0; i--)
				for (auto e2 : niveles_anteriores[i])
					anota_funcion(e1, e2, tam);
		}
	}
	else {
		// funcion - funciones del nivel anterior
		vector<info> posibles_entradas;
		// funciones usadas en este nivel
		unordered_set<unsigned int> usadas_nivel;
		vector<unsigned int> nivel_anterior = niveles_anteriores[nivel_actual - 1];
		int puertas_arriba = tam - puertas_niv_ant;

		// obtenemos las posibles entradas al próximo nivel

		// nivel anterior con nivel anterior
		for (int i = 0; i < nivel_anterior.size(); i++) {
			unsigned int funcionComputa = funcion_computa(nivel_anterior[i], nivel_anterior[i]);
			if (funciones_usadas.count(funcionComputa) == 0 && usadas_nivel.count(funcionComputa) == 0) {
				info inf;
				inf.funcion = funcionComputa;
				inf.hayPorUsar = false;
				inf.usadosNivelAnt.push_back(nivel_anterior[i]);
				posibles_entradas.push_back(inf);
				usadas_nivel.insert(funcionComputa);
			}

			for (int j = i + 1; j < nivel_anterior.size(); j++) {
				funcionComputa = funcion_computa(nivel_anterior[i], nivel_anterior[j]);
				if (funciones_usadas.count(funcionComputa) == 0 && usadas_nivel.count(funcionComputa) == 0) {
					info inf;
					inf.funcion = funcionComputa;
					inf.hayPorUsar = false;
					inf.usadosNivelAnt.push_back(nivel_anterior[i]); inf.usadosNivelAnt.push_back(nivel_anterior[j]);
					posibles_entradas.push_back(inf);
					usadas_nivel.insert(funcionComputa);
				}
			}
		}

		// nivel anterior con todos los niveles anteriores
		for (int i = 0; i < nivel_anterior.size(); i++) {
			for (int j = 0; j < nivel_actual - 1; j++) {
				vector<unsigned int> nivel_j = niveles_anteriores[j];
				for (auto fun : nivel_j) {
					unsigned int funcionComputa = funcion_computa(nivel_anterior[i], fun);
					if (funciones_usadas.count(funcionComputa) == 0 && usadas_nivel.count(funcionComputa) == 0) {
						info inf;
						inf.funcion = funcionComputa;
						if (por_usar.count(fun)) {
							inf.hayPorUsar = true;
							inf.porUsar = fun;
						}
						else
							inf.hayPorUsar = false;
						inf.usadosNivelAnt.push_back(nivel_anterior[i]);
						posibles_entradas.push_back(inf);
						usadas_nivel.insert(funcionComputa);
					}
				}
			}
		}

		// generamos las combinaciones de posibles entradas y descartamos las no factibles

		// primer nivel
		if (nivel_actual == 1) {
			vector<vector<info>> combinaciones;
			vector<info> data(estructura[nivel_actual - 1]);
			generaCombinaciones(posibles_entradas, posibles_entradas.size(), estructura[nivel_actual - 1], 0, data, 0, combinaciones);
			for (vector<info> combinacion_entrada : combinaciones) {
				auto niveles_anteriores_copy = niveles_anteriores;
				auto funciones_usadas_copy = funciones_usadas;
				combinaciones_totales++;
				vector<unsigned int> add;
				for (auto inf : combinacion_entrada) {
					add.push_back(inf.funcion);
					funciones_usadas_copy.insert(inf.funcion);
				}
				niveles_anteriores_copy.push_back(add);
				genera_circuitos(estructura, vacios, por_usar, nivel_actual + 1, niveles_anteriores_copy, tam, funciones_usadas_copy, puertas_niv_ant + estructura[nivel_actual - 1]);
			}
		}
		// resto de niveles
		else {
			// calculamos los que podemos dejar vacios y los que debemos usar
			vector<vector<info>> combinaciones;
			vector<info> data(estructura[nivel_actual - 1]);
			generaCombinaciones(posibles_entradas, posibles_entradas.size(), estructura[nivel_actual - 1], 0, data, 0, combinaciones);
			for (vector<info> combinacion_entrada : combinaciones) {
				unordered_set<unsigned int> por_usar2 = por_usar;
				unordered_set<unsigned int> usados_nivel_anterior;
				combinaciones_totales++;

				for (auto inf : combinacion_entrada) {
					// si lo uso, lo elimino de por_usar
					if (inf.hayPorUsar)
						por_usar2.erase(inf.porUsar);

					// calculo cuantos uso del nivel anterior
					for (auto& val : inf.usadosNivelAnt) {
						usados_nivel_anterior.insert(val);
					}

				}

				// vemos si la combinacion es viable
				int porUsar = por_usar2.size() + (estructura[nivel_actual - 2] - usados_nivel_anterior.size());
				if (porUsar <= vacios[nivel_actual - 1]) {
					for (auto fun : nivel_anterior) {
						if (!usados_nivel_anterior.count(fun))
							por_usar2.insert(fun);
					}
					auto niveles_anteriores_copy = niveles_anteriores;
					auto funciones_usadas_copy = funciones_usadas;
					vector<unsigned int> add;
					for (auto inf : combinacion_entrada) {
						add.push_back(inf.funcion);
						funciones_usadas_copy.insert(inf.funcion);
					}
					niveles_anteriores_copy.push_back(add);
					genera_circuitos(estructura, vacios, por_usar2, nivel_actual + 1, niveles_anteriores_copy, tam, funciones_usadas_copy, puertas_niv_ant + estructura[nivel_actual - 1]);
				}
				else {
					combinaciones_descartadas++;
				}
			}
		}
	}
}


/*
calculamos la función asociada a cada variable de entrada
*/
void input_values() {
	int group = 1;

	for (int i = 0; i < N; i++) {
		int power = pow(2, N) - 1;
		unsigned int dec = 0;
		while (power >= 0) {
			for (int j = 0; j < group; j++) {
				dec += pow(2, power);
				power--;
			}
			// 0s
			power -= group;
		}
		group *= 2;
		s_nivel_0.insert(dec);
		v_nivel_0.push_back(dec);
	}
}

void inicializa() {
	input_values();

	for (auto val : v_nivel_0) {
		func_tam[val] = 0;
		fs_ft << val << " " << 0 << "\n";
	}

	cout << "------ Tam 0 ------\n";
	cout << "Circuitos creados " << circuitos_creados << "\n";
	cout << "Numero de funciones computadas por circuitos de tamano 0: " << func_tam.size() << "\n";
	fs << "------ Tam 0 ------\n";
	fs << "Circuitos creados " << circuitos_creados << "\n";
	fs << "Numero de funciones computadas por circuitos de tamano 0: " << func_tam.size() << "\n";

	// caso tam = 1
	// nivel anterior con nivel anterior
	for (unsigned int i = 0; i < N; i++) {
		for (unsigned int j = i; j < N; j++) {
			unsigned int funcionComputa = funcion_computa(v_nivel_0[i], v_nivel_0[j]);
			++circuitos_creados;
			if (func_tam.count(funcionComputa) == 0) {
				func_tam[funcionComputa] = 1;
				fs_ft << funcionComputa << " " << 1 << "\n";
			}
		}
	}
	cout << "------ Tam 1 ------\n";
	cout << "Circuitos creados " << circuitos_creados << "\n";
	cout << "Numero de funciones computadas por circuitos de tamano 1: " << func_tam.size() - N << "\n";
	fs << "------ Tam 1 ------\n";
	fs << "Circuitos creados " << circuitos_creados << "\n";
	fs << "Numero de funciones computadas por circuitos de tamano 1: " << func_tam.size() - N << "\n";
}

bool eq_vector(const vector<int>& v1, const vector<int>& v2) {
	if (v1.size() != v2.size()) return false;
	for (int i = 0; i < v1.size(); i++)
		if (v1[i] != v2[i]) return false;
	return true;
}

/*
genera circuitos en orden creciente de tamaño
*/
void genera_circuitos_todos_tam() {
	vector<vector<int>> estructuras_anteriores = { {1} };
	bool valido = false;
	for (int i = 2; i < maxTam; i++) {
		int func_tam_ant = func_tam.size(), func_tam_ant_ind = func_tam.size();
		int circuitos_creados_ant = circuitos_creados, circuitos_creados_ant_indi = circuitos_creados;
		estructuras_anteriores = genera_estructuras(i, estructuras_anteriores);
		cout << "------ Tam " << i << " ------\n";
		fs << "------ Tam " << i << " ------\n";
		unsigned t0, t1, t0_ind, t1_ind;
		t0 = clock();
		for (auto estructura : estructuras_anteriores) {
			if (!check_condicion5(estructura)) break;
			t0_ind = clock();
			vector<int> vacios = calcula_vacios(estructura);
			unordered_set<unsigned int> por_usar;
			vector<vector<unsigned int>> niveles_anteriores = { v_nivel_0 };
			unordered_set<unsigned int> funciones_usadas = s_nivel_0;

			genera_circuitos(estructura, vacios, por_usar, 1, niveles_anteriores, i, funciones_usadas, 0);

			cout << "Estructura ";
			for (auto& val : estructura)
				cout << val << " ";
			cout << "\n";
			cout << "Combinaciones generadas " << combinaciones_totales << "\n";
			cout << "Combinaciones descartadas " << combinaciones_descartadas << "\n";
			fs << "Estructura ";
			for (auto& val : estructura)
				fs << val << " ";
			fs << "\n";
			fs << "Combinaciones generadas " << combinaciones_totales << "\n";
			fs << "Combinaciones descartadas " << combinaciones_descartadas << "\n";
			cout << "Numero de funciones computadas: " << func_tam.size() - func_tam_ant_ind << "\n";
			fs << "Numero de funciones computadas: " << func_tam.size() - func_tam_ant_ind << "\n";
			cout << "Circuitos creados " << circuitos_creados - circuitos_creados_ant_indi << "\n";
			fs << "Circuitos creados " << circuitos_creados - circuitos_creados_ant_indi << "\n";
			t1_ind = clock();
			double time = (double(t1_ind - t0_ind) / CLOCKS_PER_SEC);
			fs << "Tiempo de ejecucion: " << time << "\n";
			cout << "Tiempo de ejecucion: " << time << "\n";
			func_tam_ant_ind = func_tam.size();
			circuitos_creados_ant_indi = circuitos_creados;
			combinaciones_totales = 0;
			combinaciones_descartadas = 0;
		}
		cout << "Total:\n";
		cout << "Circuitos creados " << circuitos_creados - circuitos_creados_ant << "\n";
		cout << "Numero de funciones computadas por circuitos de tamano " << i << ": " << func_tam.size() - func_tam_ant << "\n";
		fs << "Total:\n";
		fs << "Circuitos creados " << circuitos_creados - circuitos_creados_ant << "\n";
		fs << "Numero de funciones de computadas por circuitos de tamano " << i << ": " << func_tam.size() - func_tam_ant << "\n";
		t1 = clock();
		double time = (double(t1 - t0) / CLOCKS_PER_SEC);
		fs << "Tiempo de ejecucion: " << time << "\n";
		cout << "Tiempo de ejecucion: " << time << "\n";
		func_tam_ant = func_tam.size();
		circuitos_creados_ant = circuitos_creados;

		combinaciones_totales = 0;
		combinaciones_descartadas = 0;
	}
}

/*
recibe el fichero con las funciones computadas y la última estructura utilizada para reanudar la ejecución
*/
void continua_ejecucion(const string filename, const vector<int>& last_estr) {
	// actualiza el mapa
	ifstream input(filename);
	unsigned short f;  short size;

	while (input >> f) {
		input >> size;
		func_tam[f] = size;
	}

	// continua la ejecucion desde la ultima estructura
	vector<vector<int>> estructuras_anteriores = { {1} };
	int tam = last_estr.size();
	for (int i = 2; i <= tam; i++) {
		estructuras_anteriores = genera_estructuras(i, estructuras_anteriores);
	}
	
	bool valido = false;
	for (int i = tam; i < maxTam; i++) {
		int func_tam_ant = func_tam.size(), func_tam_ant_ind = func_tam.size();
		int circuitos_creados_ant = circuitos_creados, circuitos_creados_ant_indi = circuitos_creados;
		estructuras_anteriores = genera_estructuras(i, estructuras_anteriores);
		cout << "------ Tam " << i << " ------\n";
		fs << "------ Tam " << i << " ------\n";
		unsigned t0, t1, t0_ind, t1_ind;
		t0 = clock();
		for (auto estructura : estructuras_anteriores) {
			if (eq_vector(estructura, last_estr)) valido = true;
				if (valido) {
					t0_ind = clock();
						vector<int> vacios = calcula_vacios(estructura);
						unordered_set<unsigned int> por_usar;
						vector<vector<unsigned int>> niveles_anteriores = { v_nivel_0 };
						unordered_set<unsigned int> funciones_usadas = s_nivel_0;

						genera_circuitos(estructura, vacios, por_usar, 1, niveles_anteriores, i, funciones_usadas, 0);

						cout << "Estructura ";
						for (auto& val : estructura)
							cout << val << " ";
						cout << "\n";
						cout << "Combinaciones generadas " << combinaciones_totales << "\n";
						cout << "Combinaciones descartadas " << combinaciones_descartadas << "\n";
						fs << "Estructura ";
						for (auto& val : estructura)
							fs << val << " ";
						fs << "\n";
						fs << "Combinaciones generadas " << combinaciones_totales << "\n";
						fs << "Combinaciones descartadas " << combinaciones_descartadas << "\n";
						cout << "Numero de funciones computadas: " << func_tam.size() - func_tam_ant_ind << "\n";
						fs << "Numero de funciones computadas: " << func_tam.size() - func_tam_ant_ind << "\n";
						cout << "Circuitos creados " << circuitos_creados - circuitos_creados_ant_indi << "\n";
						fs << "Circuitos creados " << circuitos_creados - circuitos_creados_ant_indi << "\n";
						t1_ind = clock();
						double time = (double(t1_ind - t0_ind) / CLOCKS_PER_SEC);
						fs << "Tiempo de ejecucion: " << time << "\n";
						cout << "Tiempo de ejecucion: " << time << "\n";
						func_tam_ant_ind = func_tam.size();
						circuitos_creados_ant_indi = circuitos_creados;
						combinaciones_totales = 0;
						combinaciones_descartadas = 0;
				}
		}
		cout << "Total:\n";
		cout << "Circuitos creados " << circuitos_creados - circuitos_creados_ant << "\n";
		cout << "Numero de funciones computadas por circuitos de tamano " << i << ": " << func_tam.size() - func_tam_ant << "\n";
		fs << "Total:\n";
		fs << "Circuitos creados " << circuitos_creados - circuitos_creados_ant << "\n";
		fs << "Numero de funciones de computadas por circuitos de tamano " << i << ": " << func_tam.size() - func_tam_ant << "\n";
		t1 = clock();
		double time = (double(t1 - t0) / CLOCKS_PER_SEC);
		fs << "Tiempo de ejecucion: " << time << "\n";
		cout << "Tiempo de ejecucion: " << time << "\n";
		func_tam_ant = func_tam.size();
		circuitos_creados_ant = circuitos_creados;

		combinaciones_totales = 0;
		combinaciones_descartadas = 0;
	}
}

int main() {
	unsigned t0, t1;
	char tam_concreto;
	cout << "Nombre del fichero de salida info: ";
	cin >> nombreFicheroInfo;
	fs = ofstream(nombreFicheroInfo);
	cout << "Nombre del fichero de salida funtam: ";
	cin >> nombreFicheroFun;
	fs_ft = ofstream(nombreFicheroFun);
	cout << "Generar circuitos a partir de un fichero y una estructura (Y/N): ";
	cin >> tam_concreto;

	if (tam_concreto == tolower('Y')) {
		string name;
		cout << "Escriba el nombre del fichero: ";
		cin >> name;
		cout << "Escriba la ultima estructura: ";
		vector<int> estr; int num;
		while (cin >> num)
			estr.push_back(num);
		t0 = clock();
		inicializa();
		continua_ejecucion(name, estr);
	}
	else {
		cout << "Elija el tamano maximo de circuitos a generar (-1 si no se quiere establecer un maximo): ";
		cin >> maxTam;
		if (maxTam == -1) maxTam = INT_MAX;
		t0 = clock();
		inicializa();
		genera_circuitos_todos_tam();
	}


	cout << "------ RESULTADOS TOTALES ------\n";
	cout << "Total de funciones distintas computadas: " << func_tam.size() << '\n';
	cout << "Total de circuitos creados: " << circuitos_creados << '\n';
	fs << "------ RESULTADOS TOTALES ------\n";
	fs << "Total de funciones distintas computadas: " << func_tam.size() << '\n';
	fs << "Total de circuitos creados: " << circuitos_creados << '\n';

	t1 = clock();
	double time = (double(t1 - t0) / CLOCKS_PER_SEC);
	fs << "Tiempo de ejecucion: " << time << "\n";
	cout << "Tiempo de ejecucion: " << time << "\n";
	fs.close();
	fs_ft.close();

	return 0;
}