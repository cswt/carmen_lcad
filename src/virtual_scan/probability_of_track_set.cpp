#include <carmen/carmen.h>
#include <carmen/moving_objects_messages.h>
#include <carmen/matrix.h>
#include <carmen/kalman.h>
#include "virtual_scan.h"


double
sum_of_tracks_lengths(virtual_scan_track_set_t *track_set)
{
	double sum = 0.0;

	for (int i = 0; i < track_set->size; i++)
		sum += track_set->tracks[i]->size;

	return ((sum > 0.0)? sum / sqrt((double) track_set->size): 0.0);
//	return ((sum > 0.0)? sum / (1.0 + log((double) track_set->size)): 0.0);
//	return (sum);
}


double
sum_of_measurements_that_fall_inside_object_models_in_track_set(virtual_scan_track_set_t *track_set)
{
	double sum = 0.0;

	for (int i = 0; i < track_set->size; i++)
	{
		for (int j = 0; j < track_set->tracks[i]->size; j++)
			sum += track_set->tracks[i]->box_model_hypothesis[j].c3;
	}

	return (sum);
}


double
sum_of_number_of_non_maximal_measurements_that_fall_behind_the_object_model(virtual_scan_track_set_t *track_set)
{
	double sum = 0.0;

	for (int i = 0; i < track_set->size; i++)
	{
		for (int j = 0; j < track_set->tracks[i]->size; j++)
			sum += track_set->tracks[i]->box_model_hypothesis[j].c2;
	}

	return (sum);
}


double
sum_of_dn_of_tracks(virtual_scan_track_set_t *track_set)
{
	double sum = 0.0;

	for (int i = 0; i < track_set->size; i++)
	{
		for (int j = 0; j < track_set->tracks[i]->size; j++)
			sum += track_set->tracks[i]->box_model_hypothesis[j].dn;
	}

	return (sum);
}


double
sum_of_variances_old(virtual_scan_track_set_t *track_set)
{
	double *variance_of_v = (double *) calloc(track_set->size, sizeof(double));
	double *variance_of_d_theta = (double *) calloc(track_set->size, sizeof(double));

	for (int i = 0; i < track_set->size; i++)
	{
		if ((track_set->tracks[i]->size - 1) > 1)
		{
			double average_v = 0.0;
			double average_d_theta = 0.0;

			for (int j = 1; j < track_set->tracks[i]->size; j++)
			{
				double distance_travelled = DIST2D(track_set->tracks[i]->box_model_hypothesis[j].hypothesis, track_set->tracks[i]->box_model_hypothesis[j - 1].hypothesis);
				double delta_theta = carmen_normalize_theta(track_set->tracks[i]->box_model_hypothesis[j].hypothesis.theta - track_set->tracks[i]->box_model_hypothesis[j - 1].hypothesis.theta);
				double delta_t = track_set->tracks[i]->box_model_hypothesis[j].hypothesis_points.precise_timestamp - track_set->tracks[i]->box_model_hypothesis[j - 1].hypothesis_points.precise_timestamp;
				double v = distance_travelled / delta_t;
				double w = delta_theta / delta_t;
				track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.v = v;
				track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.w = w;
				average_v += v;
				average_d_theta += w;
			}
			average_v /= (double) (track_set->tracks[i]->size - 1);
			average_d_theta /= (double) (track_set->tracks[i]->size - 1);

			for (int j = 1; j < track_set->tracks[i]->size; j++)
			{
				variance_of_v[i] += carmen_square(track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.v - average_v);
				variance_of_d_theta[i] += carmen_square(track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.w - average_d_theta);
			}
			variance_of_v[i] /= (double) (track_set->tracks[i]->size - 1);
			variance_of_d_theta[i] /= (double) (track_set->tracks[i]->size - 1);
		}
		else
		{
			variance_of_v[i] = 1.0;
			variance_of_d_theta[i] = 1.0;
		}
	}

	double sum = 0.0;
	for (int i = 0; i < track_set->size; i++)
		sum += variance_of_v[i] + variance_of_d_theta[i];

	sum /= (double) track_set->size;

	free(variance_of_v);
	free(variance_of_d_theta);

	return (sum);
}


double
sum_of_variances(virtual_scan_track_set_t *track_set)
{
	double *variance_of_v = (double *) calloc(track_set->size, sizeof(double));
	double *variance_of_d_theta = (double *) calloc(track_set->size, sizeof(double));

	for (int i = 0; i < track_set->size; i++)
	{
		if ((track_set->tracks[i]->size - 1) > 1)
		{
			double average_v = 0.0;
			double average_w = 0.0;

			for (int j = 1; j < track_set->tracks[i]->size; j++)
			{
				average_v += track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.v;
				average_w += track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.w;
			}
			average_v /= (double) (track_set->tracks[i]->size - 1);
			average_w /= (double) (track_set->tracks[i]->size - 1);

			for (int j = 1; j < track_set->tracks[i]->size; j++)
			{
				variance_of_v[i] += carmen_square(track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.v - average_v);
				variance_of_d_theta[i] += carmen_square(track_set->tracks[i]->box_model_hypothesis[j].hypothesis_state.w - average_w);
			}
			variance_of_v[i] /= (double) (track_set->tracks[i]->size - 1);
			variance_of_d_theta[i] /= (double) (track_set->tracks[i]->size - 1);
		}
		else
		{
			variance_of_v[i] = 1.0;
			variance_of_d_theta[i] = 1.0;
		}
	}

	double sum = 0.0;
	for (int i = 0; i < track_set->size; i++)
		sum += variance_of_v[i] + variance_of_d_theta[i];

	sum /= (double) track_set->size;

	free(variance_of_v);
	free(variance_of_d_theta);

	return (sum);
}


double
sum_of_confidences(virtual_scan_track_set_t *track_set)
{
	double sum = 0.0;

	for (int i = 0; i < track_set->size; i++)
	{
		if (track_set->tracks[i]->box_model_hypothesis->hypothesis_state.imm != NULL)
			sum += track_set->tracks[i]->box_model_hypothesis->hypothesis_state.imm_confidence;
	}

//	sum /= (double) track_set->size;

	return (sum);
}


double
probability_of_track_set_given_measurements(virtual_scan_track_set_t *track_set, bool print)
{
#define lambda_L	0.8
#define lambda_T	0.0 // 0.75 // 0.1
#define lambda_1	1.0
#define lambda_2	1.0
#define lambda_3	0.0
#define lambda_alberto	0.0 // 5.0

	if (track_set == NULL)
		return (0.0);

	double Slen = sum_of_tracks_lengths(track_set);
	double Smot = sum_of_variances(track_set);
	double Sms1 = sum_of_dn_of_tracks(track_set);
	double Sms2 = sum_of_number_of_non_maximal_measurements_that_fall_behind_the_object_model(track_set);
	double Sms3 = sum_of_measurements_that_fall_inside_object_models_in_track_set(track_set);

	double Salb = sum_of_confidences(track_set);

	if (print)
		printf("Slen = %lf, Smot = %lf, Sms1 = %lf, Sms2 = %lf, Sms3 = %lf, Salb = %lf\n",
				Slen * lambda_L, Smot * lambda_T, Sms1 * lambda_1, Sms2 * lambda_2, Sms3 * lambda_3, Salb * lambda_alberto);

	double p_w_z = exp(lambda_L * Slen - lambda_T * Smot - lambda_1 * Sms1 - lambda_2 * Sms2 - lambda_3 * Sms3 + lambda_alberto * Salb);

	return (p_w_z);
}

