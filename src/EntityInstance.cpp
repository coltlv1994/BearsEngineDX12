#include <EntityInstance.h>

void Instance::_updateModelMatrix()
{
	m_modelMatrix = XMMatrixScalingFromVector(m_scale);
	m_modelMatrix = XMMatrixRotationRollPitchYawFromVector(m_rotation) * m_modelMatrix;
	m_modelMatrix = XMMatrixTranslationFromVector(m_position) * m_modelMatrix;
}