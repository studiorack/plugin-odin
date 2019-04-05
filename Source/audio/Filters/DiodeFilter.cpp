#include "DiodeFilter.h"

DiodeFilter::DiodeFilter(void)
{
	// init
	m_k = 0.0;
	m_gamma = 0.0;

	// feedback scalars
	m_sg1 = 0.0;
	m_sg2 = 0.0;
	m_sg3 = 0.0;
	m_sg4 = 0.0;

	m_LPF1.setLP(); //LP
	m_LPF2.setLP(); //LP
	m_LPF3.setLP(); //LP
	m_LPF4.setLP(); //LP

	m_aux_control = 0.3;

	reset();
}

DiodeFilter::~DiodeFilter(void)
{
}

void DiodeFilter::reset()
{
	m_LPF1.reset();
	m_LPF2.reset();
	m_LPF3.reset();
	m_LPF4.reset();
}

void DiodeFilter::update()
{
	//modulation
	Filter::update();

	//calc alphas
	double wd = 2 * 3.141592653 * m_freq_modded;
	double t = 1.0 / m_samplerate;
	double wa = (2.0 / t) * tan(wd * t / 2.0);
	double g = wa * t / 2.0;

	double G4 = 0.5 * g / (1.0 + g);
	double G3 = 0.5 * g / (1.0 + g - 0.5 * g * G4);
	double G2 = 0.5 * g / (1.0 + g - 0.5 * g * G3);
	double G1 = g / (1.0 + g - g * G2);
	m_gamma = G4 * G3 * G2 * G1;

	m_sg1 = G4 * G3 * G2;
	m_sg2 = G4 * G3;
	m_sg3 = G4;
	m_sg4 = 1.0;

	double G = g / (1.0 + g);

	m_LPF1.m_alpha = G;
	m_LPF2.m_alpha = G;
	m_LPF3.m_alpha = G;
	m_LPF4.m_alpha = G;

	m_LPF1.m_beta = 1.0 / (1.0 + g - g * G2);
	m_LPF2.m_beta = 1.0 / (1.0 + g - 0.5 * g * G3);
	m_LPF3.m_beta = 1.0 / (1.0 + g - 0.5 * g * G4);
	m_LPF4.m_beta = 1.0 / (1.0 + g);

	m_LPF1.m_delta = g;
	m_LPF2.m_delta = 0.5 * g;
	m_LPF3.m_delta = 0.5 * g;
	m_LPF4.m_delta = 0.0;

	m_LPF1.m_gamma = 1.0 + G1 * G2;
	m_LPF2.m_gamma = 1.0 + G2 * G3;
	m_LPF3.m_gamma = 1.0 + G3 * G4;
	m_LPF4.m_gamma = 1.0;

	m_LPF1.m_epsilon = G2;
	m_LPF2.m_epsilon = G3;
	m_LPF3.m_epsilon = G4;
	m_LPF4.m_epsilon = 0.0;

	m_LPF1.m_a_0 = 1.0;
	m_LPF2.m_a_0 = 0.5;
	m_LPF3.m_a_0 = 0.5;
	m_LPF4.m_a_0 = 0.5;
}

double DiodeFilter::doFilter(double xn)
{
	m_LPF4.m_feedback = 0.0;
	m_LPF3.m_feedback = m_LPF4.getFeedbackOutput();
	m_LPF2.m_feedback = m_LPF3.getFeedbackOutput();
	m_LPF1.m_feedback = m_LPF2.getFeedbackOutput();

	double sigma = m_sg1 * m_LPF1.getFeedbackOutput() + m_sg2 * m_LPF2.getFeedbackOutput() + m_sg3 * m_LPF3.getFeedbackOutput() + m_sg4 * m_LPF4.getFeedbackOutput();

	// for passband gain compensation:
	xn *= 1.0 + m_aux_control * m_k;

	double u = (xn - m_k * sigma) / (1.0 + m_k * m_gamma);

	//todo copy saturation from ladderfilter
	// TODO real approx?
	if (m_overdrive < 1.)
	{
		//interpolate here so we have possibility of pure linear Processing
		u = u * (1. - m_overdrive) + m_overdrive * fasttanh(u);
	}
	else
	{
		u = fasttanh(m_overdrive * u);
	}

	return m_LPF4.doFilter(m_LPF3.doFilter(m_LPF2.doFilter(m_LPF1.doFilter(u))));
}

void DiodeFilter::setResControl(double res)
{
	m_k = 17.0 * res;
}