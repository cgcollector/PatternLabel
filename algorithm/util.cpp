#include "util.h"
#undef min
#undef max
namespace ldp
{
	bool getAllFilesInDir(const std::string& path,
		std::vector<std::string>& names, std::string ext)
	{
		if (ext.size())
		{
			if (ext[0] == '.')
				ext = ext.substr(1, ext.length() - 1);
		}

		WIN32_FIND_DATAA fdFile;
		HANDLE hFind = NULL;

		//Specify a file mask. *.* = We want everything!
		std::string sPathStr = ldp::fullfile(path, "*.*");
		sPathStr = ldp::validWindowsPath(sPathStr);

		if ((hFind = FindFirstFileA(sPathStr.c_str(), &fdFile)) == INVALID_HANDLE_VALUE)
		{
			printf("Path not found: [%s]\n", path.c_str());
			return false;
		}

		do
		{
			//Find first file will always return "."
			//    and ".." as the first two directories.
			if (strcmp(fdFile.cFileName, ".") != 0
				&& strcmp(fdFile.cFileName, "..") != 0)
			{
				//Build up our file path using the passed in
				//  [sDir] and the file/foldername we just found:
				sPathStr = ldp::fullfile(path, fdFile.cFileName);

				//Is the entity a File or Folder?
				if (fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
				{
					//printf("Directory: %s\n", sPathStr.c_str());
					getAllFilesInDir(sPathStr.c_str(), names, ext); //Recursion, I love it!
				}
				else if(sPathStr.substr(sPathStr.size()-ext.size(), ext.size()) == ext || ext == "*"){
					names.push_back(sPathStr);
				}
			}
		} while (FindNextFileA(hFind, &fdFile)); //Find the next file.

		FindClose(hFind); //Always, Always, clean things up!

		return true;
	}

	bool getAllFilesInDir(const std::wstring& path,
		std::vector<std::wstring>& names, std::wstring ext)
	{
		if (ext.size())
		{
			if (ext[0] == '.')
				ext = ext.substr(1, ext.length() - 1);
		}

		WIN32_FIND_DATAW fdFile;
		HANDLE hFind = NULL;

		//Specify a file mask. *.* = We want everything!
		std::wstring sPathStr = ldp::fullfile(path, L"*.*");
		sPathStr = ldp::validWindowsPath(sPathStr);

		if ((hFind = FindFirstFileW(sPathStr.c_str(), &fdFile)) == INVALID_HANDLE_VALUE)
		{
			printf("Path not found: [%s]\n", path.c_str());
			return false;
		}

		do
		{
			//Find first file will always return "."
			//    and ".." as the first two directories.
			if (lstrcmpW(fdFile.cFileName, L".") != 0
				&& lstrcmpW(fdFile.cFileName, L"..") != 0)
			{
				//Build up our file path using the passed in
				//  [sDir] and the file/foldername we just found:
				sPathStr = ldp::fullfile(path, fdFile.cFileName);

				//Is the entity a File or Folder?
				if (fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
				{
					//printf("Directory: %s\n", sPathStr.c_str());
					getAllFilesInDir(sPathStr.c_str(), names, ext); //Recursion, I love it!
				}
				else if (sPathStr.substr(sPathStr.size() - ext.size(), ext.size()) == ext || ext == L"*")
				{
					names.push_back(sPathStr);
				}
			}
		} while (FindNextFileW(hFind, &fdFile)); //Find the next file.

		FindClose(hFind); //Always, Always, clean things up!

		return true;
	}

	void kmeansCenterPP(const Mat& data, std::vector<Vec>& _out_centers, int K, int trials=3)
	{
		const int N = data.cols();
		const int nDim = data.rows();

		std::vector<int> centers(K);
		std::vector<real> _dist(N * 3);
		real* dist = _dist.data(), *tdist = dist + N, *tdist2 = tdist + N;
		real sum0 = 0;

		centers[0] = (unsigned)rand() % N;

		for (int i = 0; i < N; i++)
		{
			dist[i] = (data.col(i) - data.col(centers[0])).lpNorm<2>();
			sum0 += dist[i];
		}

		for (int k = 1; k < K; k++)
		{
			real bestSum = std::numeric_limits<real>::max();
			int bestCenter = -1;

			for (int j = 0; j < trials; j++)
			{
				real p = ((real)rand()) / real(RAND_MAX *sum0), s = 0;

				int best_i = 0;
				for (best_i = 0; best_i < N - 1; best_i++)
				if ((p -= dist[best_i]) <= 0)
					break;
				int ci = best_i;
				for (int i = 0; i < N; i++)
				{
					tdist2[i] = std::min((data.col(i) - data.col(ci)).norm(), dist[i]);
					s += tdist2[i];
				}

				if (s < bestSum)
				{
					bestSum = s;
					bestCenter = ci;
					std::swap(tdist, tdist2);
				}
			}
			centers[k] = bestCenter;
			sum0 = bestSum;
			std::swap(dist, tdist);
		}

		for (int k = 0; k < K; k++)
		{
			if (centers[k] < 0)
				throw std::runtime_error("kmeans init failed!");
			_out_centers[k] = data.col(centers[k]);
		}
	}

	void kmeans(const Mat& Data, int K, std::vector<int>& dataClusterId,
		std::vector<Vec>& dataCenters,
		int nMaxIter, bool useRandInit, bool showInfo)
	{
		const int nData = Data.cols();
		const int nDim = Data.rows();
		if (K< 0 || K>nData)
			throw std::runtime_error("K outof range in kmeans()");

		// prepare
		dataClusterId.resize(nData, -1);

		Mat distMat;
		distMat.resize(nData, K);
		distMat.setZero();

		std::vector<Vec> oldCenterPoints;
		std::vector<int> counters(K);
		dataCenters.resize(K);
		for (int i = 0; i < K; i++)
			dataCenters[i].resize(nDim);
		oldCenterPoints = dataCenters;

		real max_center_shift = 0;
		Vec temp;
		temp.resize(nDim);


		// kmeans iteration
		for (int iter = 0; iter < nMaxIter; iter++)
		{
			if (iter == 0)
			{
				if (useRandInit)
				{
					for (int i = 0; i < K; i++)
					{
						int pid = rand() % nData;
						dataCenters[i] = Data.col(i);
					}
					if (showInfo)
						printf("kmeans: rand init\n");
				}
				else
				{
					kmeansCenterPP(Data, dataCenters, K);
					if (showInfo)
						printf("kmeans: best init\n");
				}
			}//end if iter==0
			else
			{
				// compute centers
				std::fill(counters.begin(), counters.end(), int(0));
				for (int i_c = 0; i_c < dataCenters.size(); i_c++)
					dataCenters[i_c].setZero();

				for (int i_data = 0; i_data < nData; i_data++)
				{
					int k = dataClusterId[i_data];
					dataCenters[k] += Data.col(i_data);
					counters[k]++;
				}

				if (iter > 0)
					max_center_shift = 0;

				for (int k = 0; k < K; k++)
				{
					if (counters[k] != 0)
						continue;

					// if some cluster appeared to be empty then:
					//   1. find the biggest cluster
					//   2. find the farthest from the center point in the biggest cluster
					//   3. exclude the farthest point from the biggest cluster and form a new 1-point cluster.
					int max_k = 0;
					for (int k1 = 1; k1 < K; k1++)
					{
						if (counters[max_k] < counters[k1])
							max_k = k1;
					}

					double max_dist = 0;
					int farthest_i = -1;
					Vec& new_center = dataCenters[k];
					Vec& old_center = dataCenters[max_k];
					Vec& _old_center = temp; // normalized
					//float scale = 1.f/counters[max_k];
					_old_center /= real(counters[max_k]);

					for (int i_data = 0; i_data < nData; i_data++)
					{
						if (dataClusterId[i_data] != max_k)
							continue;
						const Vec& sample = Data.col(i_data);
						real dist = (sample - _old_center).norm();

						if (max_dist <= dist)
						{
							max_dist = dist;
							farthest_i = i_data;
						}
					}

					counters[max_k]--;
					counters[k]++;
					dataClusterId[farthest_i] = k;
					const Vec& sample = Data.col(farthest_i);

					old_center -= sample;
					new_center += sample;
				}//end for k

				for (int k = 0; k < K; k++)
				{
					Vec& center = dataCenters[k];
					center /= real(counters[k]);
					const Vec& old_center = oldCenterPoints[k];
					real dist = (center - old_center).norm();
					max_center_shift = std::max(max_center_shift, dist);
				}//end for k
			}//end else iter!=0

			// assign labels
			int numChanged = 0;
			for (int i_data = 0; i_data < nData; i_data++)
			{
				const Vec& sample = Data.col(i_data);
				int k_best = 0;
				real min_dist = std::numeric_limits<real>::max();

				for (int k = 0; k < K; k++)
				{
					const Vec& center = dataCenters[k];
					real dist = (center - sample).norm();
					if (min_dist > dist)
					{
						min_dist = dist;
						k_best = k;
					}
				}//end for k
				if (dataClusterId[i_data] != k_best)
					numChanged++;
				dataClusterId[i_data] = k_best;
			}//end for i, assign labels

			if (showInfo)
				printf("kmeans: iter = %d, changed = %d\n", iter, numChanged);
			if (numChanged == 0)
				break;
		}//end for iter
	}
}