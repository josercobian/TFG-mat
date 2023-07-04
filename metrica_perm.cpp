#include <iostream>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <time.h>
#include <fstream>
#include <random>
#include <string>
using namespace std;

struct hash_fn
{
	std::size_t operator() (const vector<int>& V) const
	{
		size_t H = 0;
		for (int i = 0; i < V.size(); i++)
			H ^= V[i];

		return H;
	}
};

// mapa que asocia a cada cadena el numero de permutaciones distintas
unordered_map<vector<int>, int, hash_fn> cadena_size;

/*
asocia a cada trasposición los cambios de posiciones en la cadena que representa la función booleana
*/
unordered_map<vector<int>, vector<pair<int, int>>, hash_fn> create_map(const int N) {
	unordered_map<vector<int>, vector<pair<int, int>>, hash_fn> mapa;

	for (int k = N - 2; k >= 0; k--) {
		int size = 0;
		int ini = pow(2, k), fin = pow(2, N - 1);
		while (size < pow(2, N - 2)) {
			for (int i = 0; i < pow(2, k); i++) {
				mapa[{1, N - k}].push_back({ ini + i, fin + i }); ++size;
			}
			ini += pow(2, k + 1);
			fin += pow(2, k + 1);
		}
	}
	// hallar demas trasposiciones
	vector<pair<int, int>> res;
	for (int i = 2; i < N; i++) {
		for (int j = i + 1; j <= N; j++) {
			res.insert(res.begin(), mapa[{1, i}].begin(), mapa[{1, i}].end());
			res.insert(res.begin(), mapa[{1, j}].begin(), mapa[{1, j}].end());
			res.insert(res.begin(), mapa[{1, i}].begin(), mapa[{1, i}].end());
			mapa[{i, j}] = res;
			res.clear();
		}
	}

	return mapa;
}

/*
expresa los ciclos como producto de traspociones
*/
vector<vector<int>> to_transpositions(vector<int>& perm) {
	vector<bool> visited(perm.size(), false);
	vector<vector<int>> cycle_list;

	// obtenemos los ciclos
	for (int i = 1; i <= perm.size(); i++) {
		if (!visited[i - 1]) {
			visited[i - 1] = true;
			int tmp = i;
			vector<int> cycle;
			while (i != perm[tmp - 1]) {
				cycle.push_back(tmp);
				tmp = perm[tmp - 1];
				visited[tmp - 1] = true;
			}
			if (tmp != i) {
				cycle.push_back(tmp); cycle_list.push_back(cycle);
			}
		}
	}
	// pasamos los ciclos a trasposiciones
	vector<vector<int>> trasps;
	for (auto& ciclo : cycle_list) {
		for (int i = 0; i < ciclo.size() - 1; i++) {
			//trasps.push_back({ ciclo[i],ciclo[i + 1] });
			// para que siempre sea (i,j) con i<j
			if (ciclo[i] < ciclo[i + 1])
				trasps.push_back({ ciclo[i],ciclo[i + 1] });
			else
				trasps.push_back({ ciclo[i + 1],ciclo[i] });
		}
	}

	return trasps;
}

/*
dada una cadena, devuelve el valor de M, es decir, el número de permutaciones distintas
*/
int perm_generator(vector<int>& cadena, const int N, unordered_map<vector<int>, vector<pair<int, int>>, hash_fn>& mapa) {
	vector<int> x;
	for (int i = 1; i <= N; i++)
		x.push_back(i);
	// coger diccionario trasposicion - cambio
	unordered_set<vector<int>, hash_fn> perm_set;
	vector<int> vaux;
	do {
		vector<vector<int>> trasps_list = to_transpositions(x);
		vaux.insert(vaux.begin(), cadena.begin(), cadena.end());
		for (auto& trasp : trasps_list) {
			// aplicar los cambios a cadena
			for (auto& change : mapa[trasp]) {
				auto aux = vaux[change.first];
				vaux[change.first] = vaux[change.second];
				vaux[change.second] = aux;
			}
		}
		// meter cadena en un set
		perm_set.insert(vaux);
		vaux.clear();
	} while (std::next_permutation(x.begin(), x.end()));

	int size = perm_set.size();
	// anadimos al diccionario
	for (auto& val : perm_set) {
		cadena_size[val] = size;
	}

	return size;
}

/*
dado un tamaño, calcula las posiciones correspondientes a sus funciones proyectadas
*/
vector<unordered_set<int>> generate_projections(const int N) {
	// almacenamos las posiciones que resultan al fijar cada una de las variables
	vector<unordered_set<int>> projections(N);

	int power = 0;
	while (power < N) {
		for (int i = 0; i < pow(2, N); i += 2*pow(2,power)) {
			int j;
			for (j = 0; j < pow(2,power); j++) 
				projections[power].insert(j+i);
		}
		cout << endl;
		power++;
	}

	return projections;
}

/*
calcula el valor de la métrica M' para una función
*/
int M_prime(vector<int>& f, const int N) {
	auto mapa_N = create_map(N);
	// Calculamos M
	int sum = (cadena_size.count(f) ? cadena_size[f] : perm_generator(f, N, mapa_N));

	// Calculamos las proyecciones
	auto mapa_N_1 = create_map(N - 1);
	vector<unordered_set<int>> projections = generate_projections(N);
	for (auto& proj : projections) {
		vector<int> f_p, f_p_neg;
		for (int i = 0; i < pow(2, N); i++) {
			if (proj.count(i)) f_p.push_back(f[i]);
			else f_p_neg.push_back(f[i]);
		}
		sum += perm_generator(f_p, N - 1, mapa_N_1);
		sum += perm_generator(f_p_neg, N - 1, mapa_N_1);
	}

	return sum;
}