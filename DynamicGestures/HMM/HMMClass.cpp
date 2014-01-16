#include "HMMClass.h"

HMMClass::HMMClass(int _K, int _N, int _M){
	K = _K;
	N = _N;
	M = _M;

	pi_ptr = boost::shared_ptr< HMMVector<double> >(new HMMVector<double>(K));
	T_ptr = boost::shared_ptr< HMMMatrix<double> >(new HMMMatrix<double>(K, K));
	E_ptr = boost::shared_ptr< HMMMatrix<double> >(new HMMMatrix<double>(M, K));

	HMMVector<double> &pi = *pi_ptr;
	HMMMatrix<double> &T = *T_ptr;
	HMMMatrix<double> &E = *E_ptr;


	// initial probabilities
	pi(0) = 1;
	for (int i = 1; i < K; i++)
		pi(i) = 0;

	// Initial structure of sequential states
	// 0.5 to connection to itself and the next one
	for (int i = 0; i < K; i++) {
		for (int j = 0; j < K; j++) {
			if (j == i || (i + 1) % K == j) {
				T(i, j) = 0.5;
			} else
				T(i, j) = 0.0;
		}
	}
	T(K - 1, K - 1) = 1.0;
	T(K - 1, 0) = 0.0;

	// Initial random emission matrix
	for (int i = 0; i < M; i++) {
		double x = 0.0;
		for (int j = 0; j < K; j++) {
			E(i, j) = rand();
			x += E(i, j);
		}
		for (int j = 0; j < K; j++) {
			E(i, j) /= x;
		}
	}

	hmm = new HMM<double>(pi_ptr, T_ptr, E_ptr);
}

void HMMClass::train(std::vector<sequence> trainDataset, int crossValK,
		int iterationNumber, double learningRate) {
	cout<<"!"<<endl;
	boost::shared_ptr<HMMVector<double> > pi2_ptr(new HMMVector<double>(K));
	boost::shared_ptr<HMMMatrix<double> > T2_ptr(new HMMMatrix<double>(K, K));
	boost::shared_ptr<HMMMatrix<double> > E2_ptr(new HMMMatrix<double>(M, K));

	HMMVector<double> &pi2 = *pi2_ptr;
	HMMMatrix<double> &T2 = *T2_ptr;
	HMMMatrix<double> &E2 = *E2_ptr;

	cout<<"!!"<<endl;
	HMMVector<double> &pi = *pi_ptr;
	HMMMatrix<double> &T = *T_ptr;
	HMMMatrix<double> &E = *E_ptr;

	cout<<"!!!"<<endl;
	HMMMatrix<double> F(N, K);
	HMMVector<double> scales(N);
	HMMMatrix<double> B(N, K);
	sequence hiddenseq(N);

	KFoldCrossValidation<sequence> kFoldCV(trainDataset, crossValK);
	double total_best = 0.0;
	cout<<"!!!!"<<endl;
	for (int i = 0; i < iterationNumber; i++) {
		for (int foldNumber = 0; foldNumber < kFoldCV.getK(); foldNumber++) {
			for (int elIndex = 0;
					elIndex < kFoldCV.getLearningSetSize(foldNumber);
					elIndex++) {
				sequence elValue = kFoldCV.getLearningSetElement(elIndex,
						foldNumber);

//				cout<<"OBS : ";
//				 for (int i=0;i<elValue.size();i++) {
//				 cout<<elValue[i]<<" ";
//				 }
//				 cout<<endl;

				double loglik = hmm->viterbi(elValue, hiddenseq);
				std::cout << "-- log likelihood of hiddenseq: " << loglik
						<< "\tLikelihood : " << exp(loglik) << std::endl;
//				cout << "STATES : ";
//				 for (int i = 0; i < hiddenseq.size(); i++) {
//				 cout << hiddenseq[i] << " ";
//				 }
//				 std::cout<<endl;

				//std::cout << "Running forward" << std::endl;
				hmm->forward(elValue, scales, F);

				//std::cout << "Running likelihood" << std::endl;
				//loglik = hmm->likelihood(scales);

				//std::cout << "Running backward" << std::endl;
				hmm->backward(elValue, scales, B);

				//std::cout << "Running posterior decoding" << std::endl;
				//hmm->posterior_decoding(elValue, F, B, scales, pd);

				//std::cout << "Running Baum-Welch" << std::endl;
				hmm->baum_welch(elValue, F, B, scales, *pi2_ptr, *T2_ptr,
						*E2_ptr);

//				for (int j = 0; j < K; j++) {
//
//				 for (int k = 0; k < M; k++) {
//				 std::cout << E2(j, k) << " ";
//				 }
//				 std::cout << std::endl;
//				 }
//
//				 for (int j = 0; j < K; j++) {
//
//				 for (int k = 0; k < K; k++) {
//				 std::cout << T2(j, k) << " ";
//				 }
//				 std::cout << std::endl;
//
//				 }

				// Update from learning
				for (int j = 0; j < K; j++) {

					double sum = 0.0;
					for (int k = 0; k < M; k++) {
						E(k, j) = (1.0 - learningRate) * E(k, j)
								+ learningRate * E2(k, j);
						sum += E(k, j);
					}
					for (int k = 0; k < M; k++) {
						E(k, j) /= sum;
					}

					sum = 0.0;
					for (int k = 0; k < K; k++) {
						T(j, k) = (1.0 - learningRate) * T(j, k)
								+ learningRate * T2(j, k);
						sum += T(j, k);
					}
					for (int k = 0; k < K; k++) {
						T(j, k) /= sum;
					}

				}
				delete hmm;
				hmm = new HMM<double>(pi_ptr, T_ptr, E_ptr);
			}

			// Checking stop condition
			double total_rec = 0;
			for (int elIndex = 0;
					elIndex < kFoldCV.getTestingSetSize(foldNumber);
					elIndex++) {
				sequence elValue = kFoldCV.getTestingSetElement(elIndex,
						foldNumber);

				double loglik = hmm->viterbi(elValue, hiddenseq);
				total_rec += exp(loglik);
			}

			if (total_rec <= total_best)
				break;

			total_best = total_rec;
		}
	}
}

double HMMClass::predict(sequence test)
{
	sequence hiddenseq(N);
	return hmm->viterbi(test, hiddenseq);
}

void HMMClass::show()
{
	HMMVector<double> &pi = *pi_ptr;
	HMMMatrix<double> &T = *T_ptr;
	HMMMatrix<double> &E = *E_ptr;

	std::cout << " ------ " << std::endl;
	std::cout << " Proposed model ! " << std::endl;
	std::cout << " -> Initial probabilites " << std::endl;
	for (int j = 0; j < K; j++)
		std::cout << (pi)(j) << " ";
	std::cout << std::endl;

	std::cout << " -> Transition matrix " << std::endl;
	for (int j = 0; j < K; j++) {

		for (int k = 0; k < K; k++) {
			std::cout << T(j, k) << " ";
		}
		std::cout << std::endl;

	}

	std::cout << " -> Emission matrix " << std::endl;

	for (int j = 0; j < K; j++) {

		for (int k = 0; k < M; k++) {
			std::cout << E(j, k) << " ";
		}
		std::cout << std::endl;

	}
}
